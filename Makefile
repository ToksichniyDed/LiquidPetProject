include .env
export

DBMATE_IMAGE := ghcr.io/amacneil/dbmate
MIGRATIONS_DIR := $(shell pwd)/order-service/db/migrations

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

.PHONY: db-up db-down db-logs db-ps \
        migrate-new migrate-up migrate-down migrate-status

## Поднять Postgres в фоне
db-up:
	docker compose up -d postgres

## Остановить и удалить контейнеры + volume (чистый старт)
db-down:
	docker compose down -v

## Логи Postgres
db-logs:
	docker compose logs postgres

## Статус контейнеров (включая остановленные)
db-ps:
	docker compose ps -a

## Создать новый файл миграции: make migrate-new name=create_orders
migrate-new:
	$(DBMATE) new $(name)

## Применить все неприменённые миграции
migrate-up:
	$(DBMATE_WITH_DB) up

## Откатить последнюю применённую миграцию
migrate-down:
	$(DBMATE_WITH_DB) down

## Показать, какие миграции применены
migrate-status:
	$(DBMATE_WITH_DB) status
