//
// Created by DED on 13.09.2026.
//

#include <PostgresOrderRepository.h>
#include <gtest/gtest.h>
#include <models/DatabaseConfiguration.h>

#include <cstdlib>
#include <pqxx/pqxx>

using namespace order_system::models;
using namespace order_system::repository;
using namespace shared::models;

namespace {

Order buildTestOrder(const std::string& userId, const std::string& productId) {
    auto currency = Currency::create("RUB", 2).value();
    auto price = Money::create(500, currency).value();
    auto product = ProductId::create(productId).value();
    auto item = OrderItem::create(product, 2, price).value();
    auto user = UserId::create(userId).value();

    std::vector<OrderItem> items;
    items.push_back(item);

    return Order::create(user, std::move(items)).value();
}

}  // namespace

class PostgresOrderRepositoryIntegrationTest : public ::testing::Test {
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

        // Полная очистка перед каждым тестом
        pqxx::work cleanup(*rawConnection);
        cleanup.exec("TRUNCATE TABLE orders CASCADE");
        cleanup.exec("TRUNCATE TABLE outbox");
        cleanup.commit();

        repository = std::make_unique<PostgresOrderRepository>(*config);
    }

    static constexpr auto USER_ID = "22222222-2222-2222-2222-222222222222";

    static constexpr auto PRODUCT_ID = "11111111-1111-1111-1111-111111111111";

    static constexpr auto NON_EXISTENT_ORDER_ID = "99999999-9999-9999-9999-999999999999";

    std::unique_ptr<DatabaseConfiguration> config;
    std::unique_ptr<pqxx::connection> rawConnection;
    std::unique_ptr<PostgresOrderRepository> repository;
};

TEST_F(PostgresOrderRepositoryIntegrationTest, SavePersistsOrderRetrievableByFindById) {
    auto order = buildTestOrder(USER_ID, PRODUCT_ID);

    auto saveResult = repository->save(order);
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    auto found = repository->findById(saveResult.value());
    ASSERT_TRUE(found.has_value()) << found.error().message();

    EXPECT_EQ(found->userId(), order.userId());

    ASSERT_EQ(found->items().size(), 1);

    EXPECT_EQ(found->items()[0].productId(), order.items()[0].productId());

    EXPECT_EQ(found->items()[0].quantity(), order.items()[0].quantity());

    EXPECT_EQ(found->items()[0].priceAtOrderTime().minorUnits(), 500);

    EXPECT_EQ(found->status(), Order::OrderStatus::Created);
}

TEST_F(PostgresOrderRepositoryIntegrationTest, FindByIdReturnsNotFoundForUnknownId) {
    auto id = OrderId::create(NON_EXISTENT_ORDER_ID).value();

    auto result = repository->findById(id);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RepositoryError::NotFound);
}

TEST_F(PostgresOrderRepositoryIntegrationTest, SaveActuallyWritesRowsVisibleViaRawSql) {
    // Данные для findById мы готовим через save(), но здесь проверяем
    // сам факт записи напрямую SQL-запросом — не полагаясь на findById(),
    // чтобы не проверять save() и findById() друг через друга.
    auto order = buildTestOrder(USER_ID, PRODUCT_ID);

    auto saveResult = repository->save(order);
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    pqxx::work check(*rawConnection);

    auto orderRows = check.exec(
        "SELECT user_id, status "
        "FROM orders "
        "WHERE order_id = " +
        check.quote(saveResult.value().value()));

    check.commit();

    ASSERT_EQ(orderRows.size(), 1);

    EXPECT_EQ(orderRows[0]["user_id"].as<std::string>(), USER_ID);

    EXPECT_EQ(orderRows[0]["status"].as<std::string>(), "Created");
}

TEST_F(PostgresOrderRepositoryIntegrationTest, SaveWritesOutboxEventInSameTransaction) {
    auto order = buildTestOrder(USER_ID, PRODUCT_ID);

    auto saveResult = repository->save(order);
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    pqxx::work check(*rawConnection);

    auto outboxRows = check.exec(
        "SELECT aggregate_id, event_type, published "
        "FROM outbox "
        "WHERE aggregate_id = " +
        check.quote(saveResult.value().value()));

    check.commit();

    ASSERT_EQ(outboxRows.size(), 1);

    EXPECT_EQ(outboxRows[0]["event_type"].as<std::string>(), "OrderCreated");

    EXPECT_EQ(outboxRows[0]["published"].as<bool>(), false);
}

TEST_F(PostgresOrderRepositoryIntegrationTest, SaveGeneratesDifferentOrderIdsForSameUser) {
    auto firstOrder = buildTestOrder(USER_ID, PRODUCT_ID);
    auto secondOrder = buildTestOrder(USER_ID, PRODUCT_ID);

    auto firstResult = repository->save(firstOrder);
    auto secondResult = repository->save(secondOrder);

    ASSERT_TRUE(firstResult.has_value());
    ASSERT_TRUE(secondResult.has_value());

    EXPECT_NE(firstResult.value(), secondResult.value());
}

TEST_F(PostgresOrderRepositoryIntegrationTest, SaveWithMultipleItemsPersistsAllOfThem) {
    auto currency = Currency::create("RUB", 2).value();

    auto product1 = ProductId::create(PRODUCT_ID).value();

    auto product2 = ProductId::create("33333333-3333-3333-3333-333333333333").value();

    auto item1 = OrderItem::create(product1, 2, Money::create(500, currency).value()).value();

    auto item2 = OrderItem::create(product2, 1, Money::create(1500, currency).value()).value();

    auto user = UserId::create(USER_ID).value();

    auto order = Order::create(user, {item1, item2}).value();

    auto saveResult = repository->save(order);

    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    auto found = repository->findById(saveResult.value());

    ASSERT_TRUE(found.has_value()) << found.error().message();

    EXPECT_EQ(found->items().size(), 2);
}

TEST_F(PostgresOrderRepositoryIntegrationTest, FindByIdReturnsStatusStoredInDatabase) {
    auto saveResult = repository->save(buildTestOrder(USER_ID, PRODUCT_ID));
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    pqxx::work update(*rawConnection);
    update.exec("UPDATE orders SET status = 'Reserved' WHERE order_id = " +
                update.quote(saveResult.value().value()));
    update.commit();

    auto found = repository->findById(saveResult.value());

    ASSERT_TRUE(found.has_value()) << found.error().message();
    EXPECT_EQ(found->status(), Order::OrderStatus::Reserved);
}

TEST_F(PostgresOrderRepositoryIntegrationTest, ChangeStatusAppliesTransitionWhenStatusMatches) {
    auto saveResult = repository->save(buildTestOrder(USER_ID, PRODUCT_ID));
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    auto changed = repository->changeStatus(saveResult.value(), Order::OrderStatus::Created,
                                            Order::OrderStatus::Reserved);

    ASSERT_TRUE(changed.has_value()) << changed.error().message();
    EXPECT_TRUE(changed.value());

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT status FROM orders WHERE order_id = " + check.quote(saveResult.value().value()));
    check.commit();
    EXPECT_EQ(rows[0]["status"].as<std::string>(), "Reserved");
}

TEST_F(PostgresOrderRepositoryIntegrationTest, ChangeStatusDoesNothingWhenStatusDiffers) {
    auto saveResult = repository->save(buildTestOrder(USER_ID, PRODUCT_ID));
    ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();

    pqxx::work update(*rawConnection);
    update.exec("UPDATE orders SET status = 'Cancelled' WHERE order_id = " +
                update.quote(saveResult.value().value()));
    update.commit();

    auto changed = repository->changeStatus(saveResult.value(), Order::OrderStatus::Created,
                                            Order::OrderStatus::Reserved);

    ASSERT_TRUE(changed.has_value()) << changed.error().message();
    EXPECT_FALSE(changed.value());

    pqxx::work check(*rawConnection);
    auto rows = check.exec("SELECT status FROM orders WHERE order_id = " + check.quote(saveResult.value().value()));
    check.commit();
    EXPECT_EQ(rows[0]["status"].as<std::string>(), "Cancelled");
}

TEST_F(PostgresOrderRepositoryIntegrationTest, ChangeStatusReturnsFalseForUnknownOrder) {
    auto id = OrderId::create(NON_EXISTENT_ORDER_ID).value();

    auto changed = repository->changeStatus(id, Order::OrderStatus::Created, Order::OrderStatus::Reserved);

    ASSERT_TRUE(changed.has_value()) << changed.error().message();
    EXPECT_FALSE(changed.value());
}
