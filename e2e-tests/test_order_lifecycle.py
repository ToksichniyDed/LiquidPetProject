"""
E2E: POST /orders через gateway -> order-service -> Postgres -> outbox
     -> Kafka(orders.created) -> worker-service -> worker-postgres.

Система должна быть заранее поднята через `docker compose up --wait`
и миграции применены.
"""

from helpers import (
    make_order_payload,
    poll_for_outbox_event,
    poll_for_outbox_published,
    poll_for_processed_event,
    poll_for_order_status
)


def test_order_created_via_gateway_is_processed_by_worker(
        http_session, gateway_url, order_db_connection, worker_db_connection
):
    # Arrange
    payload = make_order_payload()

    # Act — создаём заказ через публичную точку входа системы
    response = http_session.post(f"{gateway_url}/orders", json=payload, timeout=5)

    # Assert — ближний: gateway/order-service отработали синхронную часть
    assert response.status_code == 201, response.text
    order_id = response.json()["orderId"]
    assert order_id

    # Assert — outbox-запись создана в той же транзакции, что и заказ
    event_id = poll_for_outbox_event(order_db_connection, order_id)
    assert event_id is not None, "outbox-запись для заказа не появилась вовремя"

    # Assert — OutboxPublisher реально доставил событие в Kafka
    published = poll_for_outbox_published(order_db_connection, event_id)
    assert published, "outbox-запись не была помечена published=true вовремя"

    # Assert — дальний: worker-service забрал событие из Kafka и обработал его
    processed = poll_for_processed_event(worker_db_connection, event_id)
    assert processed, "worker-service не обработал событие вовремя"

    # Assert — цикл замкнут: order-service прочитал orders.reserved и перевёл заказ в Reserved
    reserved_order = poll_for_order_status(http_session, gateway_url, order_id, "Reserved")
    assert reserved_order is not None, "заказ не перешёл в статус Reserved вовремя"


def test_created_order_is_retrievable_via_get(http_session, gateway_url):
    # Arrange
    payload = make_order_payload(quantity=3, price_minor_units=2500)

    # Act
    create_response = http_session.post(f"{gateway_url}/orders", json=payload, timeout=5)
    assert create_response.status_code == 201, create_response.text
    order_id = create_response.json()["orderId"]

    get_response = http_session.get(f"{gateway_url}/orders/{order_id}", timeout=5)

    # Assert
    assert get_response.status_code == 200, get_response.text
    order = get_response.json()
    assert order["orderId"] == order_id
    assert order["userId"] == payload["userId"]
    assert len(order["items"]) == 1
    assert order["items"][0]["quantity"] == 3
    assert order["status"] in ("Created", "Reserved")
