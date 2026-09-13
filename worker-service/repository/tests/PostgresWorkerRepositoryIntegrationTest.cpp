//
// Created by DED on 14.09.2026.
//

// Интеграционный тест: требует поднятого Postgres (см. docker-compose.yaml,
// сервис "worker-postgres", порт 5433) и применённых миграций
// (make migrate-up service=worker-service).
// Адрес берётся из переменной окружения WORKER_SERVICE_DATABASE_URL.

#include <gtest/gtest.h>
#include <models/DatabaseConfiguration.h>

#include <cstdlib>
#include <pqxx/pqxx>

#include "../PostgresWorkerRepository.h"

using namespace worker_service::repository;
using namespace shared::models;

namespace {

ReservationRecord makeRecord(const std::string& eventId, const std::string& aggregateId,
                             const std::string& eventType = "OrderReserved",
                             const std::string& payload = R"({"orderId":"test"})") {
    return ReservationRecord{
        .eventId = OutboxEventId::create(eventId).value(),
        .aggregateId = OrderId::create(aggregateId).value(),
        .eventType = eventType,
        .payload = payload,
    };
}

}  // namespace

class PostgresWorkerRepositoryIntegrationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        const char* url = std::getenv("WORKER_SERVICE_DATABASE_URL");

        if (!url) {
            GTEST_SKIP() << "WORKER_SERVICE_DATABASE_URL не задан — "
                            "пропускаем интеграционный тест. "
                            "Пример: postgres://worker:worker@localhost:5433/worker?sslmode=disable";
        }

        auto parsedConfig = DatabaseConfiguration::fromUrl(url);

        if (!parsedConfig.has_value()) {
            GTEST_SKIP() << "Не удалось распарсить WORKER_SERVICE_DATABASE_URL: " << parsedConfig.error().message();
        }

        config = std::make_unique<DatabaseConfiguration>(std::move(parsedConfig.value()));

        rawConnection = std::make_unique<pqxx::connection>(config->toConnectionString());

        pqxx::work cleanup(*rawConnection);
        cleanup.exec("TRUNCATE TABLE processed_order_events");
        cleanup.exec("TRUNCATE TABLE outbox");
        cleanup.commit();

        repository = std::make_unique<PostgresWorkerRepository>(*config);
    }

    static constexpr auto EVENT_ID = "11111111-1111-1111-1111-111111111111";
    static constexpr auto AGGREGATE_ID = "22222222-2222-2222-2222-222222222222";

    std::unique_ptr<DatabaseConfiguration> config;
    std::unique_ptr<pqxx::connection> rawConnection;
    std::unique_ptr<PostgresWorkerRepository> repository;
};

TEST_F(PostgresWorkerRepositoryIntegrationTest, NewEventReturnsTrue) {
    auto record = makeRecord(EVENT_ID, AGGREGATE_ID);

    auto result = repository->recordReservationIfNew(record);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result.value());
}

TEST_F(PostgresWorkerRepositoryIntegrationTest, NewEventActuallyWritesProcessedEventRow) {
    auto record = makeRecord(EVENT_ID, AGGREGATE_ID);

    auto result = repository->recordReservationIfNew(record);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT event_id FROM processed_order_events WHERE event_id = " + check.quote(EVENT_ID));
    check.commit();

    EXPECT_EQ(rows.size(), 1);
}

TEST_F(PostgresWorkerRepositoryIntegrationTest, NewEventActuallyWritesOutboxRow) {
    auto record = makeRecord(EVENT_ID, AGGREGATE_ID, "OrderReserved", R"({"orderId":"abc"})");

    auto result = repository->recordReservationIfNew(record);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT aggregate_id, event_type, payload FROM outbox WHERE aggregate_id = " +
                           check.quote(AGGREGATE_ID));
    check.commit();

    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows[0]["event_type"].as<std::string>(), "OrderReserved");
    EXPECT_EQ(rows[0]["payload"].as<std::string>(), R"({"orderId": "abc"})");
}

TEST_F(PostgresWorkerRepositoryIntegrationTest, DuplicateEventIdReturnsFalse) {
    auto record = makeRecord(EVENT_ID, AGGREGATE_ID);

    auto first = repository->recordReservationIfNew(record);
    ASSERT_TRUE(first.has_value()) << first.error().message();
    ASSERT_TRUE(first.value());

    auto second = repository->recordReservationIfNew(record);

    ASSERT_TRUE(second.has_value()) << second.error().message();
    EXPECT_FALSE(second.value());
}

TEST_F(PostgresWorkerRepositoryIntegrationTest, DuplicateEventIdDoesNotCreateSecondOutboxRow) {
    auto record = makeRecord(EVENT_ID, AGGREGATE_ID);

    auto first = repository->recordReservationIfNew(record);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(first.value());

    auto second = repository->recordReservationIfNew(record);
    ASSERT_TRUE(second.has_value());
    ASSERT_FALSE(second.value());

    // Атомарность: раз dedup-вставка не прошла, аборт транзакции должен был
    // откатить и вторую (outbox) вставку - outbox не должен вырасти.
    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT COUNT(*) AS cnt FROM outbox WHERE aggregate_id = " + check.quote(AGGREGATE_ID));
    check.commit();

    EXPECT_EQ(rows[0]["cnt"].as<int>(), 1);
}

TEST_F(PostgresWorkerRepositoryIntegrationTest, DifferentEventIdsForSameAggregateBothSucceed) {
    auto firstRecord = makeRecord(EVENT_ID, AGGREGATE_ID, "OrderReserved", R"({"attempt":1})");
    auto secondRecord =
        makeRecord("33333333-3333-3333-3333-333333333333", AGGREGATE_ID, "OrderReserved", R"({"attempt":2})");

    auto first = repository->recordReservationIfNew(firstRecord);
    auto second = repository->recordReservationIfNew(secondRecord);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_TRUE(first.value());
    EXPECT_TRUE(second.value());

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT COUNT(*) AS cnt FROM outbox WHERE aggregate_id = " + check.quote(AGGREGATE_ID));
    check.commit();

    EXPECT_EQ(rows[0]["cnt"].as<int>(), 2);
}
