"""
Сценарий нагрузки на order-system через gateway.

Виртуальный пользователь создаёт заказы (POST /orders) и читает свои же заказы
(GET /orders/{id}), поэтому читаются только существующие id.

Имена запросов ("POST /orders", "GET /orders/[id]") используются в отчётах: без группировки по name
каждый GET с новым id был бы отдельной строкой статистики.

Параметры через переменные окружения:
    LOAD_HOST        адрес gateway (по умолчанию http://localhost:8081)
    LOAD_THINK_MIN   минимальная пауза между запросами пользователя, секунды (по умолчанию 0.05)
    LOAD_THINK_MAX   максимальная пауза, секунды (по умолчанию 0.2)

Запуск ступеней с отчётом делает ladder.py; для ручного эксперимента с веб-интерфейсом:
    make load-ui
"""

import os
import random
import uuid

from locust import FastHttpUser, between, task

THINK_MIN = float(os.environ.get("LOAD_THINK_MIN", "0.05"))
THINK_MAX = float(os.environ.get("LOAD_THINK_MAX", "0.2"))

# Сколько последних созданных id помнит один пользователь
MAX_REMEMBERED_ORDERS = 50


def make_order_payload() -> dict:
    return {
        "userId": str(uuid.uuid4()),
        "items": [
            {
                "productId": str(uuid.uuid4()),
                "quantity": random.randint(1, 5),
                "priceAtOrderTime": {
                    "minorUnits": random.randint(100, 100_000),
                    "currency": {"code": "USD", "minorDigits": 2},
                },
            }
        ],
    }


class OrderUser(FastHttpUser):
    host = os.environ.get("LOAD_HOST", "http://localhost:8081")
    wait_time = between(THINK_MIN, THINK_MAX)

    # Под перегрузкой запрос должен упасть ошибкой в отчёте, а не висеть по умолчанию целую минуту
    connection_timeout = 5.0
    network_timeout = 10.0

    def on_start(self) -> None:
        self._order_ids: list[str] = []

    def _remember(self, order_id: str) -> None:
        self._order_ids.append(order_id)
        if len(self._order_ids) > MAX_REMEMBERED_ORDERS:
            self._order_ids.pop(0)

    # Соотношение 1:2 - записи в конвейере (outbox, Kafka, worker) создают только POST'ы,
    # читающая часть нужна, чтобы проверить путь gateway -> order-service -> Postgres на чтении
    @task(1)
    def create_order(self) -> None:
        with self.client.post(
                "/orders", json=make_order_payload(), name="POST /orders", catch_response=True
        ) as response:
            if response.status_code != 201:
                response.failure(f"unexpected status {response.status_code}")
                return

            try:
                order_id = response.json()["orderId"]
            except (ValueError, KeyError, TypeError):
                response.failure("response has no orderId")
                return

            self._remember(order_id)

    @task(2)
    def get_order(self) -> None:
        if not self._order_ids:
            self.create_order()
            return

        order_id = random.choice(self._order_ids)
        with self.client.get(f"/orders/{order_id}", name="GET /orders/[id]", catch_response=True) as response:
            if response.status_code != 200:
                response.failure(f"unexpected status {response.status_code}")
