//
// Created by DED on 18.08.2026.
//

#include <gtest/gtest.h>

#include "MockOrderRepository.h"

using namespace order_system::models;
using namespace order_system::repository;

class OrderRepositoryTest : public ::testing::Test {

private:
    static Order buildTestOrder() {
        auto currency = Currency::create("RUB", 2).value();
        auto price = Money::create(500, currency).value();
        auto productId = ProductId::create(kProductIdValue).value();
        auto item = OrderItem::create(productId, 2, price).value();
        auto userId = UserId::create(kUserIdValue).value();

        std::vector<OrderItem> items;
        items.push_back(item);
        return Order::create(userId, std::move(items)).value();
    }

protected:
    static constexpr const char* kProductIdValue = "11111111-1111-1111-1111-111111111111";
    static constexpr const char* kUserIdValue = "22222222-2222-2222-2222-222222222222";
    static constexpr const char* kGeneratedOrderIdValue = "33333333-3333-3333-3333-333333333333";
    static constexpr const char* kNonExistentOrderIdValue = "44444444-4444-4444-4444-444444444444";

protected:
    OrderRepositoryTest() {
    }

    MockOrderRepository _mockRepo;
    Order _testOrder = buildTestOrder();

};

TEST_F(OrderRepositoryTest, SaveReturnsGeneratedOrderIdOnSuccess) {
    auto expectedId = OrderId::create(kGeneratedOrderIdValue);
    ASSERT_TRUE(expectedId.has_value());

    EXPECT_CALL(_mockRepo, save(testing::_)).WillOnce(testing::Return(expectedId));

    auto orderId = _mockRepo.save(_testOrder);
    ASSERT_TRUE(orderId.has_value());
    EXPECT_EQ(*orderId, *expectedId);
}

TEST_F(OrderRepositoryTest, SaveReturnsConnectionFailureOnDbError) {
    EXPECT_CALL(_mockRepo, save(testing::_))
        .WillOnce(testing::Return(std::unexpected(RepositoryError::ConnectionFailure)));

    auto result = _mockRepo.save(_testOrder);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RepositoryError::ConnectionFailure);
}

TEST_F(OrderRepositoryTest, FindByIdReturnsOrderWhenExists) {
    auto lookupId = OrderId::create(kGeneratedOrderIdValue);
    ASSERT_TRUE(lookupId.has_value());

    EXPECT_CALL(_mockRepo, findById(testing::_))
        .WillOnce(testing::Return(_testOrder));

    auto result = _mockRepo.findById(*lookupId);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->userId().value(), _testOrder.userId().value());
}

TEST_F(OrderRepositoryTest, FindByIdReturnsNotFoundWhenMissing) {
    auto missingId = OrderId::create(kNonExistentOrderIdValue);
    ASSERT_TRUE(missingId.has_value());

    EXPECT_CALL(_mockRepo, findById(testing::_))
        .WillOnce(testing::Return(std::unexpected(RepositoryError::NotFound)));

    auto result = _mockRepo.findById(*missingId);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RepositoryError::NotFound);
}
