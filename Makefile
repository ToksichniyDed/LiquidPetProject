include .env
export

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

.PHONY: db-up db-up-one db-down db-logs db-ps \
        migrate-new migrate-up migrate-down migrate-status check-database-url

check-database-url:
	@if [ -z "$(DATABASE_URL)" ]; then \
		echo "Error: $(service_upper)_DATABASE_URL is not set for service='$(service)'"; \
		exit 1; \
	fi

## Поднять весь Postgres (оба сервиса) в фоне
db-up:
	docker compose up -d postgres worker-postgres

## Поднять Postgres одного сервиса: make db-up-one service=name-postgres
db-up-one:
	docker compose up -d $(service)

## Остановить и удалить контейнеры + volume (чистый старт)
db-down:
	docker compose down -v

## Логи Postgres
db-logs:
	docker compose logs postgres worker-postgres

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
