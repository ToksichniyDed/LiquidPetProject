# Переменные окружения (DSN баз) берутся из .env, который основной load-tests.mk уже подхватывает.

LOAD_VENV_DIR := load-tests/.venv
LOAD_PYTHON := $(LOAD_VENV_DIR)/bin/python

LOAD_STAGES ?= 10,50,100,200
LOAD_DURATION ?= 120
LOAD_DRAIN_TIMEOUT ?= 0
LOAD_HOST ?= http://localhost:8081

.PHONY: load-venv load-ladder load-snapshot load-drain load-ui

## Создать venv для нагрузочных тестов (idempotent)
load-venv:
	@if [ ! -x "$(LOAD_PYTHON)" ]; then \
		$(PYTHON) -m venv $(LOAD_VENV_DIR); \
		echo "venv создан в $(LOAD_VENV_DIR)"; \
	fi
	@$(LOAD_PYTHON) -m pip install --upgrade pip -q
	@$(LOAD_PYTHON) -m pip install -r load-tests/requirements.txt -q

## Ступенчатая нагрузка + замеры конвейера: make load-ladder LOAD_STAGES=10,50 LOAD_DURATION=60
## Финальный дренаж (с проверкой инварианта): LOAD_DRAIN_TIMEOUT=3600
load-ladder: check-env load-venv
	$(LOAD_PYTHON) load-tests/ladder.py --stages $(LOAD_STAGES) --duration $(LOAD_DURATION) \
		--host $(LOAD_HOST) --drain-timeout $(LOAD_DRAIN_TIMEOUT)

## Текущее состояние конвейера (JSON)
load-snapshot: check-env load-venv
	$(LOAD_PYTHON) load-tests/pipeline_stats.py snapshot

## Дождаться, пока все заказы пройдут резервирование: make load-drain LOAD_DRAIN_TIMEOUT=3600
load-drain: check-env load-venv
	$(LOAD_PYTHON) load-tests/pipeline_stats.py drain --timeout $(if $(filter 0,$(LOAD_DRAIN_TIMEOUT)),3600,$(LOAD_DRAIN_TIMEOUT))

## Веб-интерфейс Locust для ручных экспериментов: http://localhost:8089
load-ui: load-venv
	$(LOAD_VENV_DIR)/bin/locust -f load-tests/locustfile.py --host $(LOAD_HOST)
