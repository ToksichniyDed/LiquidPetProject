-include .env
export

ENV_FILE := .env

DBMATE_IMAGE := ghcr.io/amacneil/dbmate
MIGRATIONS_DIR := $(shell pwd)/$(service)/db/migrations

# name-service -> NAME_SERVICE_DATABASE_URL
service_upper := $(shell echo $(service) | tr 'a-z-' 'A-Z_')
DATABASE_URL := $($(service_upper)_DATABASE_URL)

DBMATE := docker run --rm -it \
    --user "$$(id -u):$$(id -g)" \
    -v "$(MIGRATIONS_DIR):/db/migrations" \
    $(DBMATE_IMAGE)

DBMATE_WITH_DB := docker run --rm -it \
    --user "$$(id -u):$$(id -g)" \
    --network=host \
    -v "$(MIGRATIONS_DIR):/db/migrations" \
    -e DATABASE_URL="$(DATABASE_URL)" \
    $(DBMATE_IMAGE)

.PHONY: env check-env db-up db-up-one db-down db-logs db-ps \
        migrate-new migrate-up migrate-down migrate-status check-database-url \
        venv e2e-up e2e-down e2e

## Создать .env из .env.example (существующий файл не трогает)
env:
	@if [ -f "$(ENV_FILE)" ]; then \
		echo "$(ENV_FILE) уже существует, оставляю как есть"; \
	else \
		cp .env.example $(ENV_FILE); \
		echo "$(ENV_FILE) создан из .env.example"; \
	fi

check-env:
	@if [ ! -f "$(ENV_FILE)" ]; then \
		echo "Error: нет файла $(ENV_FILE). Создай его командой: make env"; \
		exit 1; \
	fi

check-database-url: check-env
	@if [ -z "$(DATABASE_URL)" ]; then \
		echo "Error: $(service_upper)_DATABASE_URL не задан в $(ENV_FILE) для service='$(service)'"; \
		exit 1; \
	fi

## Поднять весь Postgres (оба сервиса) в фоне
db-up: check-env
	docker compose up -d --wait order-postgres worker-postgres

## Поднять Postgres одного сервиса: make db-up-one service=name-postgres
db-up-one: check-env
	docker compose up -d $(service)

## Остановить и удалить контейнеры + volume (чистый старт)
db-down:
	docker compose down -v

## Логи Postgres
db-logs:
	docker compose logs order-postgres worker-postgres

## Статус контейнеров (включая остановленные)
db-ps:
	docker compose ps -a

## Создать новый файл миграции: make migrate-new service=name-service name=name_mogrations
migrate-new:
	$(DBMATE) new $(name)

## Применить все неприменённые миграции: make migrate-up service=name-service
migrate-up: check-database-url
	$(DBMATE_WITH_DB) up

## Откатить последнюю применённую миграцию
migrate-down: check-database-url
	$(DBMATE_WITH_DB) down

## Показать, какие миграции применены
migrate-status: check-database-url
	$(DBMATE_WITH_DB) status

VENV_DIR := e2e-tests/.venv
PYTHON := python3

## Создать venv для e2e-тестов и поставить/обновить зависимости (idempotent,
## существующий venv не пересоздаёт - только доустанавливает requirements)
venv:
	@if [ ! -x "$(VENV_DIR)/bin/python" ]; then \
		$(PYTHON) -m venv $(VENV_DIR); \
		echo "venv создан в $(VENV_DIR)"; \
	fi
	@$(VENV_DIR)/bin/pip install --upgrade pip -q
	@$(VENV_DIR)/bin/pip install -r e2e-tests/requirements.txt -q

## Поднять всю систему для e2e: пересобрать образы (подхватить правки в коде)
## и дождаться, пока все сервисы со healthcheck станут healthy
e2e-up: check-env
	docker compose up -d --build --wait

## Погасить систему и удалить volume'ы
e2e-down:
	docker compose down -v

## Полный локальный e2e-прогон: поднять систему, прогнать pytest.
## Контейнеры намеренно остаются подняты после теста
e2e: e2e-up venv
	$(VENV_DIR)/bin/python -m pytest e2e-tests -v

pytest: venv
	$(VENV_DIR)/bin/python -m pytest e2e-tests -v
