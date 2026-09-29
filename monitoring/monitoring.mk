## Поднять Kafka + kafka-exporter + Prometheus + Grafana (Grafana: http://localhost:3000, admin/admin)
monitoring-up: check-env
	docker compose --profile monitoring up -d --wait kafka kafka-exporter prometheus grafana

## Погасить только мониторинг (Kafka и сервисы не трогает, volume'ы Prometheus/Grafana сохраняются)
monitoring-down:
	docker compose --profile monitoring stop kafka-exporter prometheus grafana
	docker compose --profile monitoring rm -f kafka-exporter prometheus grafana

## Логи мониторинга
monitoring-logs:
	docker compose --profile monitoring logs -f kafka-exporter prometheus grafana
