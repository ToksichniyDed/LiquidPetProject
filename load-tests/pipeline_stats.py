"""
Состояние конвейера заказов по данным БД:
order-service -> outbox -> Kafka -> worker-service -> outbox -> Kafka -> order-service.

Нагрузчик видит только HTTP-часть, до ответа 201. Здесь снимается то, что происходит после него:
сколько заказов ещё не зарезервировано, сколько записей outbox не ушло в Kafka и сколько событий
обработал worker. DSN берутся из тех же переменных окружения, что и у e2e-тестов
(ORDER_SERVICE_DATABASE_URL, WORKER_SERVICE_DATABASE_URL, см. .env.example).

Использование:
    python pipeline_stats.py snapshot
    python pipeline_stats.py drain --timeout 600
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from dataclasses import asdict, dataclass

import psycopg

ORDER_DSN_ENV = "ORDER_SERVICE_DATABASE_URL"
WORKER_DSN_ENV = "WORKER_SERVICE_DATABASE_URL"


@dataclass(frozen=True)
class PipelineSnapshot:
    orders_total: int
    orders_created: int  # статус Created: заказ ещё не прошёл резервирование
    orders_reserved: int
    order_outbox_unpublished: int  # события OrderCreated, ещё не ушедшие в Kafka
    worker_processed: int  # уникальные event_id, обработанные worker'ом
    worker_outbox_unpublished: int  # события OrderReserved, ещё не ушедшие в Kafka

    @property
    def is_drained(self) -> bool:
        """Конвейер пуст: все заказы зарезервированы, обе outbox-таблицы отправлены."""
        return (
                self.orders_created == 0
                and self.order_outbox_unpublished == 0
                and self.worker_outbox_unpublished == 0
        )


def dsns_from_env() -> tuple[str, str]:
    missing = [name for name in (ORDER_DSN_ENV, WORKER_DSN_ENV) if not os.environ.get(name)]
    if missing:
        raise SystemExit(f"Не заданы переменные окружения: {', '.join(missing)} (см. .env.example)")
    return os.environ[ORDER_DSN_ENV], os.environ[WORKER_DSN_ENV]


def take_snapshot(order_dsn: str, worker_dsn: str) -> PipelineSnapshot:
    with psycopg.connect(order_dsn, autocommit=True) as connection, connection.cursor() as cursor:
        cursor.execute("SELECT status, count(*) FROM orders GROUP BY status")
        by_status = {status: count for status, count in cursor.fetchall()}
        cursor.execute("SELECT count(*) FROM outbox WHERE published = FALSE")
        order_outbox_unpublished = cursor.fetchone()[0]

    with psycopg.connect(worker_dsn, autocommit=True) as connection, connection.cursor() as cursor:
        cursor.execute("SELECT count(*) FROM processed_order_events")
        worker_processed = cursor.fetchone()[0]
        cursor.execute("SELECT count(*) FROM outbox WHERE published = FALSE")
        worker_outbox_unpublished = cursor.fetchone()[0]

    return PipelineSnapshot(
        orders_total=sum(by_status.values()),
        orders_created=by_status.get("Created", 0),
        orders_reserved=by_status.get("Reserved", 0),
        order_outbox_unpublished=order_outbox_unpublished,
        worker_processed=worker_processed,
        worker_outbox_unpublished=worker_outbox_unpublished,
    )


def wait_until_drained(
        order_dsn: str, worker_dsn: str, timeout_seconds: float, poll_seconds: float = 1.0
) -> tuple[float | None, PipelineSnapshot]:
    """Ждёт, пока конвейер опустеет. Возвращает (секунды ожидания | None при таймауте, последний снимок).
    При timeout_seconds=0 делает один снимок и не ждёт."""
    started = time.monotonic()
    while True:
        snapshot = take_snapshot(order_dsn, worker_dsn)
        elapsed = time.monotonic() - started
        if snapshot.is_drained:
            return elapsed, snapshot
        if elapsed >= timeout_seconds:
            return None, snapshot
        time.sleep(poll_seconds)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("snapshot", help="напечатать текущее состояние конвейера (JSON)")
    drain_parser = subparsers.add_parser("drain", help="дождаться, пока конвейер опустеет")
    drain_parser.add_argument("--timeout", type=float, default=3600, help="секунды ожидания (по умолчанию 3600)")
    args = parser.parse_args()

    order_dsn, worker_dsn = dsns_from_env()

    if args.command == "snapshot":
        snapshot = take_snapshot(order_dsn, worker_dsn)
        print(json.dumps({**asdict(snapshot), "is_drained": snapshot.is_drained}, indent=2))
        return 0

    elapsed, snapshot = wait_until_drained(order_dsn, worker_dsn, args.timeout)
    if elapsed is None:
        print(f"Не дренировано за {args.timeout:.0f} с: {snapshot}")
        return 1
    print(f"Дренировано за {elapsed:.1f} с: {snapshot}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
