#!/usr/bin/env python3
"""
Ступенчатая нагрузка: для каждого числа пользователей отдельный запуск Locust (headless),
до и после него снимок конвейера по БД. В конце таблица для README и отчёты в load-tests/reports/.

Что попадает в таблицу:
  * HTTP-часть из CSV Locust: RPS, доля ошибок, p50/p95/p99 (после разгона, --reset-stats);
  * конвейер после ответа 201: сколько заказов создано за ступень, сколько из них успел обработать
    worker и сколько записей outbox ещё не ушло в Kafka.

Между ступенями дренаж не ждём: worker обрабатывает ~5 сообщений/с на инстанс, и очередь после одной
ступени может разбираться десятки минут. Вместо этого фиксируем рост очереди за ступень
(created - processed). Финальный дренаж включается флагом --drain-timeout; только после него
проверяется инвариант "число новых заказов == число новых обработанных событий".

Запуск: make load-ladder (см. load-tests.mk)
"""

from __future__ import annotations

import argparse
import csv
import importlib.metadata
import math
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from pipeline_stats import PipelineSnapshot, dsns_from_env, take_snapshot, wait_until_drained

LOAD_TESTS_DIR = Path(__file__).resolve().parent
LOCUSTFILE = LOAD_TESTS_DIR / "locustfile.py"


@dataclass(frozen=True)
class LocustStats:
    requests: int
    failures: int
    rps: float
    p50: float
    p95: float
    p99: float
    post_p95: float
    get_p95: float

    @property
    def failure_percent(self) -> float:
        return 100.0 * self.failures / self.requests if self.requests else 0.0


@dataclass(frozen=True)
class StageResult:
    users: int
    stats: LocustStats
    before: PipelineSnapshot
    after: PipelineSnapshot

    @property
    def created(self) -> int:
        return self.after.orders_total - self.before.orders_total

    @property
    def processed(self) -> int:
        return self.after.worker_processed - self.before.worker_processed

    @property
    def backlog_growth(self) -> int:
        return self.created - self.processed


def to_float(value: str | None) -> float:
    try:
        return float(value)  # type: ignore[arg-type]
    except (TypeError, ValueError):
        return math.nan


def parse_stats_csv(path: Path) -> LocustStats:
    if not path.exists():
        raise SystemExit(f"Locust не создал {path}: см. вывод выше")

    with path.open(newline="", encoding="utf-8") as file:
        rows = {row["Name"]: row for row in csv.DictReader(file)}

    total = rows["Aggregated"]
    post = next((row for name, row in rows.items() if name.startswith("POST ")), None)
    get = next((row for name, row in rows.items() if name.startswith("GET ")), None)

    return LocustStats(
        requests=int(total["Request Count"]),
        failures=int(total["Failure Count"]),
        rps=to_float(total["Requests/s"]),
        p50=to_float(total["50%"]),
        p95=to_float(total["95%"]),
        p99=to_float(total["99%"]),
        post_p95=to_float(post["95%"]) if post else math.nan,
        get_p95=to_float(get["95%"]) if get else math.nan,
    )


def run_locust_stage(users: int, args: argparse.Namespace, report_dir: Path) -> LocustStats:
    prefix = report_dir / f"stage-{users}"
    spawn_rate = max(1, math.ceil(users / args.ramp_seconds))

    command = [
        sys.executable, "-m", "locust",
        "-f", str(LOCUSTFILE),
        "--headless",
        "--host", args.host,
        "-u", str(users),
        "-r", str(spawn_rate),
        "-t", f"{args.duration}s",
        "--reset-stats",  # статистика считается после того, как все пользователи запущены
        "--only-summary",
        "--csv", str(prefix),
        "--html", f"{prefix}.html",
        "--exit-code-on-error", "0",  # ошибки запросов - это данные отчёта, а не сбой запуска
    ]

    completed = subprocess.run(command, check=False)
    if completed.returncode != 0:
        raise SystemExit(f"Locust завершился с кодом {completed.returncode} на ступени {users}")

    return parse_stats_csv(Path(f"{prefix}_stats.csv"))


def ms(value: float) -> str:
    return "n/a" if math.isnan(value) else f"{value:.0f}"


def format_table(results: list[StageResult]) -> str:
    lines = [
        "| Users | Requests | RPS | Errors | p50, ms | p95, ms | p99, ms | POST p95 | GET p95 "
        "| +Orders | +Processed | Backlog Δ | Created (не зарезервированы) | Outbox unpublished |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for result in results:
        stats = result.stats
        lines.append(
            f"| {result.users} | {stats.requests} | {stats.rps:.1f} | {stats.failure_percent:.1f}% "
            f"| {ms(stats.p50)} | {ms(stats.p95)} | {ms(stats.p99)} | {ms(stats.post_p95)} | {ms(stats.get_p95)} "
            f"| {result.created} | {result.processed} | {result.backlog_growth:+d} "
            f"| {result.after.orders_created} | {result.after.order_outbox_unpublished} |"
        )
    return "\n".join(lines)


def describe_invariant(
        baseline: PipelineSnapshot, final: PipelineSnapshot, drain_seconds: float | None, drain_timeout: float
) -> str:
    if drain_seconds is None:
        if drain_timeout == 0:
            return (
                f"Инвариант не проверялся: дренаж не запускался. Осталось заказов в статусе Created: "
                f"{final.orders_created}, outbox order: {final.order_outbox_unpublished}, "
                f"outbox worker: {final.worker_outbox_unpublished}. Дождаться: make load-drain"
            )
        return (
            f"Не дренировано за {drain_timeout:.0f} с (Created: {final.orders_created}, outbox order: "
            f"{final.order_outbox_unpublished}, outbox worker: {final.worker_outbox_unpublished}). "
            "Инвариант не проверялся."
        )

    new_orders = final.orders_total - baseline.orders_total
    new_processed = final.worker_processed - baseline.worker_processed
    verdict = "OK" if new_orders == new_processed else "НАРУШЕН"
    return (
        f"Дренаж за {drain_seconds:.0f} с. Инвариант {verdict}: новых заказов {new_orders}, "
        f"новых обработанных событий {new_processed}"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--stages", default="10,50,100,200", help="число пользователей по ступеням, через запятую")
    parser.add_argument("--duration", type=int, default=120, help="длительность ступени в секундах, включая разгон")
    parser.add_argument("--ramp-seconds", type=int, default=10, help="за сколько секунд запускаются все пользователи")
    parser.add_argument("--host", default=os.environ.get("LOAD_HOST", "http://localhost:8081"), help="адрес gateway")
    parser.add_argument("--drain-timeout", type=float, default=0, help="сколько секунд ждать пустой конвейер в конце (0 - не ждать)")
    parser.add_argument("--reports-dir", default=str(LOAD_TESTS_DIR / "reports"))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    stages = [int(part) for part in args.stages.split(",") if part.strip()]
    order_dsn, worker_dsn = dsns_from_env()

    report_dir = Path(args.reports_dir) / time.strftime("%Y%m%d-%H%M%S")
    report_dir.mkdir(parents=True)

    baseline = take_snapshot(order_dsn, worker_dsn)
    if not baseline.is_drained:
        print(
            f"ВНИМАНИЕ: конвейер не пуст перед стартом ({baseline}). Хвост прошлого прогона "
            "занимает worker и искажает Backlog Δ. Лучше: make load-drain или пересоздать стек."
        )

    results: list[StageResult] = []
    for users in stages:
        print(f"\n=== Ступень: {users} пользователей, {args.duration} с ===")
        before = take_snapshot(order_dsn, worker_dsn)
        stats = run_locust_stage(users, args, report_dir)
        after = take_snapshot(order_dsn, worker_dsn)
        results.append(StageResult(users=users, stats=stats, before=before, after=after))

    drain_seconds, final = wait_until_drained(order_dsn, worker_dsn, args.drain_timeout)

    try:
        locust_version = importlib.metadata.version("locust")
    except importlib.metadata.PackageNotFoundError:
        locust_version = "unknown"

    summary = "\n".join(
        [
            f"# Нагрузочный прогон {report_dir.name}",
            "",
            f"- host: {args.host}, ступень: {args.duration} с (разгон {args.ramp_seconds} с), locust {locust_version}",
            f"- CPU ядер на машине: {os.cpu_count()} (Locust и стек на одной машине)",
            "- сервер: connection-per-request к upstream, keep-alive только клиент -> gateway, однопоточный блокирующий HttpServer",
            "",
            format_table(results),
            "",
            describe_invariant(baseline, final, drain_seconds, args.drain_timeout),
            "",
        ]
    )
    (report_dir / "summary.md").write_text(summary, encoding="utf-8")

    print("\n" + summary)
    print(f"Отчёты: {report_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
