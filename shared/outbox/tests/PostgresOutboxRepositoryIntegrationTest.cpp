//
// Created by DED on 14.09.2026.
//

// Интеграционный тест: требует поднятого Postgres (см. docker-compose.yaml,
// сервис "postgres") и применённых миграций (make migrate-up service=order-service).
// Адрес берётся из переменной окружения ORDER_SERVICE_DATABASE_URL.

#include <PostgresOutboxRepository.h>
#include <gtest/gtest.h>
#include <models/DatabaseConfiguration.h>
#include <repository/RepositoryError.h>

#include <cstdlib>
#include <pqxx/pqxx>
#include <thread>

using namespace shared::outbox;
using namespace shared::models;

namespace {

// Вставляет запись в outbox напрямую SQL - готовим тестовые данные
std::string insertOutboxRow(pqxx::connection& connection, const std::string& aggregateId, const std::string& eventType,
                            bool published) {
    pqxx::work work(connection);
    auto row = work.exec(
        "INSERT INTO outbox (aggregate_id, event_type, payload, published) "
        "VALUES (" +
        work.quote(aggregateId) + ", " + work.quote(eventType) + ", '{}'::jsonb, " + (published ? "TRUE" : "FALSE") +
        ") "
        "RETURNING id");
    work.commit();
    return row[0]["id"].as<std::string>();
}

}  // namespace

class PostgresOutboxRepositoryIntegrationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        const char* url = std::getenv("ORDER_SERVICE_DATABASE_URL");

        if (!url) {
            GTEST_SKIP() << "ORDER_SERVICE_DATABASE_URL не задан — "
                            "пропускаем интеграционный тест. "
                            "Пример: postgres://orders:orders@localhost:5432/orders?sslmode=disable";
        }

        auto parsedConfig = DatabaseConfiguration::fromUrl(url);

        if (!parsedConfig.has_value()) {
            GTEST_SKIP() << "Не удалось распарсить ORDER_SERVICE_DATABASE_URL: " << parsedConfig.error().message();
        }

        config = std::make_unique<DatabaseConfiguration>(std::move(parsedConfig.value()));

        rawConnection = std::make_unique<pqxx::connection>(config->toConnectionString());

        pqxx::work cleanup(*rawConnection);
        cleanup.exec("TRUNCATE TABLE outbox");
        cleanup.commit();

        repository = std::make_unique<PostgresOutboxRepository>(*config);
    }

    static constexpr auto AGGREGATE_ID = "22222222-2222-2222-2222-222222222222";

    std::unique_ptr<DatabaseConfiguration> config;
    std::unique_ptr<pqxx::connection> rawConnection;
    std::unique_ptr<PostgresOutboxRepository> repository;
};

TEST_F(PostgresOutboxRepositoryIntegrationTest, FetchUnpublishedReturnsEmptyWhenOutboxIsEmpty) {
    auto result = repository->fetchUnpublished(100);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->empty());
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, FetchUnpublishedReturnsOnlyUnpublishedEntries) {
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderCreated", /*published=*/false);
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderReserved", /*published=*/true);

    auto result = repository->fetchUnpublished(100);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ(result->at(0).eventType, "OrderCreated");
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, FetchUnpublishedRespectsLimit) {
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "EventA", false);
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "EventB", false);
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "EventC", false);

    auto result = repository->fetchUnpublished(2);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->size(), 2);
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, FetchUnpublishedReturnsInCreationOrder) {
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "First", false);
    // created_at имеет разрешение TIMESTAMPTZ, небольшая пауза гарантирует
    // различимый порядок вставки, а не полагается на совпадающий timestamp.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "Second", false);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "Third", false);

    auto result = repository->fetchUnpublished(100);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ(result->at(0).eventType, "First");
    EXPECT_EQ(result->at(1).eventType, "Second");
    EXPECT_EQ(result->at(2).eventType, "Third");
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, FetchedEntryContainsAggregateIdAndPayload) {
    insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderCreated", false);

    auto result = repository->fetchUnpublished(100);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ(result->at(0).aggregateId, AGGREGATE_ID);
    EXPECT_EQ(result->at(0).payload, "{}");
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, MarkAsPublishedSucceedsForExistingEntry) {
    auto id = insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderCreated", false);
    auto entryId = OutboxEventId::create(id).value();

    auto result = repository->markAsPublished(entryId);

    ASSERT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, MarkAsPublishedReturnsNotFoundForUnknownId) {
    auto unknownId = OutboxEventId::create("99999999-9999-9999-9999-999999999999").value();

    auto result = repository->markAsPublished(unknownId);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), shared::repository::RepositoryError::NotFound);
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, MarkAsPublishedActuallyUpdatesRowVisibleViaRawSql) {
    auto id = insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderCreated", false);
    auto entryId = OutboxEventId::create(id).value();

    auto markResult = repository->markAsPublished(entryId);
    ASSERT_TRUE(markResult.has_value()) << markResult.error().message();

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT published, published_at FROM outbox WHERE id = " + check.quote(id));
    check.commit();

    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows[0]["published"].as<bool>(), true);
    EXPECT_FALSE(rows[0]["published_at"].is_null());
}

TEST_F(PostgresOutboxRepositoryIntegrationTest, MarkedEntryNoLongerReturnedByFetchUnpublished) {
    auto id = insertOutboxRow(*rawConnection, AGGREGATE_ID, "OrderCreated", false);
    auto entryId = OutboxEventId::create(id).value();

    auto markResult = repository->markAsPublished(entryId);
    ASSERT_TRUE(markResult.has_value()) << markResult.error().message();

    auto fetchResult = repository->fetchUnpublished(100);

    ASSERT_TRUE(fetchResult.has_value()) << fetchResult.error().message();
    EXPECT_TRUE(fetchResult->empty());
}
