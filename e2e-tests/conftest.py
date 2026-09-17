"""
Фикстуры pytest для e2e-тестов order-system.
Все URL/DSN читаются из переменных окружения
"""

import os

import psycopg
import pytest
import requests


def _require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(
            f"Переменная окружения {name} не задана. "
            f"Загрузите .env перед запуском e2e-тестов (см. README в e2e-tests/)."
        )
    return value


@pytest.fixture(scope="session")
def gateway_url() -> str:
    # По умолчанию - как в docker-compose.yaml (порт gateway проброшен наружу)
    return os.environ.get("E2E_GATEWAY_URL", "http://localhost:8081")


@pytest.fixture(scope="session")
def order_service_db_dsn() -> str:
    return _require_env("ORDER_SERVICE_DATABASE_URL")


@pytest.fixture(scope="session")
def worker_service_db_dsn() -> str:
    return _require_env("WORKER_SERVICE_DATABASE_URL")


@pytest.fixture()
def order_db_connection(order_service_db_dsn):
    conn = psycopg.connect(order_service_db_dsn, autocommit=True)
    try:
        yield conn
    finally:
        conn.close()


@pytest.fixture()
def worker_db_connection(worker_service_db_dsn):
    conn = psycopg.connect(worker_service_db_dsn, autocommit=True)
    try:
        yield conn
    finally:
        conn.close()


@pytest.fixture()
def http_session():
    session = requests.Session()
    try:
        yield session
    finally:
        session.close()
