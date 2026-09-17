# E2E-тесты order-system

Проверяют систему целиком как чёрный ящик: HTTP-запрос в gateway ->
вся цепочка сервисов -> проверка конечного состояния в БД.

## Быстрый запуск

Из корня репозитория:

```bash
make env   # один раз - создаёт .env из .env.example, если его ещё нет
make e2e
```

`make e2e` делает всё сам: поднимает и пересобирает docker-compose (миграции
накатываются автоматически сервисами `order-service-migrate` /
`worker-service-migrate`, ждёт healthcheck у всех сервисов), создаёт
изолированный venv в `e2e-tests/.venv` (без конфликтов с системным python -
см. PEP 668), ставит `requirements.txt` и запускает `pytest -v`.

Контейнеры после теста намеренно остаются поднятыми - удобно смотреть логи
упавшего теста через `docker compose logs`. Погасить систему и почистить
volume'ы:

```bash
make e2e-down
```

## Запуск руками



```bash
make env                              # создать .env (один раз)
make e2e-up                           # docker compose up -d --build --wait
make venv                             # создать/обновить e2e-tests/.venv

make pytest

make e2e-down                         # погасить и почистить volume'ы
```

Переменные окружения (DSN для БД, URL gateway) `make` подхватывает из `.env`
и пробрасывает в среду pytest автоматически

## Что проверяется

- `test_order_created_via_gateway_is_processed_by_worker` - полный путь
  заказа через outbox pattern и Kafka до dedup-таблицы worker-service.
- `test_created_order_is_retrievable_via_get` - синхронный путь чтения
  через gateway-прокси.
