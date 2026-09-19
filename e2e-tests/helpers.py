"""
Вспомогательные функции для e2e-тестов: построение тестовых данных
и поллинг БД с таймаутом.
"""

import uuid

from tenacity import retry, stop_after_delay, wait_fixed, retry_if_result


def make_order_payload(user_id: str | None = None, quantity: int = 2, price_minor_units: int = 1500) -> dict:
    """Собирает валидное тело POST /orders в формате, который ожидает
    OrderJsonMapper::fromJson (см. order-service/models2json-mapper)."""
    return {
        "userId": user_id or str(uuid.uuid4()),
        "items": [
            {
                "productId": str(uuid.uuid4()),
                "quantity": quantity,
                "priceAtOrderTime": {
                    "minorUnits": price_minor_units,
                    "currency": {"code": "USD", "minorDigits": 2},
                },
            }
        ],
    }


def _is_none(value) -> bool:
    return value is None


@retry(stop=stop_after_delay(10), wait=wait_fixed(0.2), retry=retry_if_result(_is_none))
def poll_for_outbox_event(connection, order_id: str) -> str | None:
    """Ждёт появления outbox-записи для данного заказа в order-postgres
    и возвращает её id (это же значение потом попадает в processed_order_events
    как event_id, см. PublishRequest.metadata.eventId)."""
    with connection.cursor() as cursor:
        cursor.execute(
            "SELECT id FROM outbox WHERE aggregate_id = %s AND event_type = %s",
            (order_id, "OrderCreated"),
        )
        row = cursor.fetchone()
        return str(row[0]) if row else None


@retry(stop=stop_after_delay(10), wait=wait_fixed(0.2), retry=retry_if_result(_is_none))
def poll_for_outbox_published(connection, event_id: str) -> bool | None:
    """Ждёт, пока OutboxPublisher реально опубликует событие в Kafka
    (published = TRUE), а не просто создаст запись."""
    with connection.cursor() as cursor:
        cursor.execute("SELECT published FROM outbox WHERE id = %s", (event_id,))
        row = cursor.fetchone()
        if row is None:
            return None
        return True if row[0] else None


@retry(stop=stop_after_delay(15), wait=wait_fixed(0.3), retry=retry_if_result(_is_none))
def poll_for_processed_event(connection, event_id: str) -> bool | None:
    """Ждёт, пока worker-service обработает событие и запишет dedup-отметку
    в processed_order_events (см. PostgresWorkerRepository::recordReservationIfNew)."""
    with connection.cursor() as cursor:
        cursor.execute(
            "SELECT 1 FROM processed_order_events WHERE event_id = %s",
            (event_id,),
        )
        row = cursor.fetchone()
        return True if row else None

@retry(stop=stop_after_delay(15), wait=wait_fixed(0.3), retry=retry_if_result(_is_none))
def poll_for_order_status(session, gateway_url: str, order_id: str, expected_status: str) -> dict | None:
    """Ждёт, пока GET /orders/{id} вернёт заказ с нужным статусом (замыкание цикла резервирования:
    worker -> orders.reserved -> order-service). Возвращает тело заказа или None."""
    response = session.get(f"{gateway_url}/orders/{order_id}", timeout=5)
    if response.status_code != 200:
        return None
    order = response.json()
    return order if order["status"] == expected_status else None
