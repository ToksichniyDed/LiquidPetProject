//
// Created by DED on 19.09.2026.
//

#include <MockOrderRepository.h>
#include <MockOrderStatusRepository.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "OrderReservedHandler.h"

using namespace order_service::event_handlers;
using namespace order_system::models;
using namespace order_system::repository;

using ::testing::_;
using ::testing::Return;

namespace {
constexpr auto VALID_ORDER_ID = "11111111-1111-1111-1111-111111111111";
constexpr auto VALID_USER_ID = "22222222-2222-2222-2222-222222222222";
constexpr auto VALID_PRODUCT_ID = "33333333-3333-3333-3333-333333333333";
constexpr auto VALID_EVENT_ID = "44444444-4444-4444-4444-444444444444";

Order buildOrder(Order::OrderStatus status) {
    auto price = Money::create(500, Currency::create("USD", 2).value()).value();
    auto item = OrderItem::create(ProductId::create(VALID_PRODUCT_ID).value(), 1, price).value();
    return Order::restore(UserId::create(VALID_USER_ID).value(), OrderId::create(VALID_ORDER_ID).value(), {item},
                          status)
        .value();
}

std::string validPayload() { return nlohmann::json{{"orderId", VALID_ORDER_ID}}.dump(); }

shared::messaging::MessageMetadata validMetadata() {
    return {.eventId = shared::models::OutboxEventId::create(VALID_EVENT_ID).value(), .eventType = "OrderReserved"};
}

OrderId orderId() { return OrderId::create(VALID_ORDER_ID).value(); }
}  // namespace

class OrderReservedHandlerTest : public ::testing::Test {
   protected:
    MockOrderRepository _orderRepository;
    MockOrderStatusRepository _statusRepository;
    OrderReservedHandler _handler{_orderRepository, _statusRepository};
};

TEST_F(OrderReservedHandlerTest, MalformedJsonIsSkippedAndCommitted) {
    EXPECT_CALL(_orderRepository, findById(_)).Times(0);
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_TRUE(_handler.handle("not valid json {{{", validMetadata()));
}

TEST_F(OrderReservedHandlerTest, EventWithoutOrderIdIsSkippedAndCommitted) {
    EXPECT_CALL(_orderRepository, findById(_)).Times(0);
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_TRUE(_handler.handle("{}", validMetadata()));
}

TEST_F(OrderReservedHandlerTest, UnknownOrderIsSkippedAndCommitted) {
    EXPECT_CALL(_orderRepository, findById(_)).WillOnce(Return(std::unexpected(RepositoryError::NotFound)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_TRUE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, RepositoryFailureOnLoadIsRetried) {
    EXPECT_CALL(_orderRepository, findById(_))
        .WillOnce(Return(std::unexpected(RepositoryError::ConnectionFailure)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_FALSE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, AlreadyReservedOrderIsIdempotentlyCommitted) {
    EXPECT_CALL(_orderRepository, findById(_)).WillOnce(Return(buildOrder(Order::OrderStatus::Reserved)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_TRUE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, CancelledOrderIsSkippedAndCommitted) {
    EXPECT_CALL(_orderRepository, findById(_)).WillOnce(Return(buildOrder(Order::OrderStatus::Cancelled)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).Times(0);

    EXPECT_TRUE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, CreatedOrderIsTransitionedToReserved) {
    EXPECT_CALL(_orderRepository, findById(orderId())).WillOnce(Return(buildOrder(Order::OrderStatus::Created)));
    EXPECT_CALL(_statusRepository,
                changeStatus(orderId(), Order::OrderStatus::Created, Order::OrderStatus::Reserved))
        .WillOnce(Return(true));

    EXPECT_TRUE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, ConcurrentStatusChangeIsRetried) {
    EXPECT_CALL(_orderRepository, findById(_)).WillOnce(Return(buildOrder(Order::OrderStatus::Created)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _)).WillOnce(Return(false));

    EXPECT_FALSE(_handler.handle(validPayload(), validMetadata()));
}

TEST_F(OrderReservedHandlerTest, RepositoryFailureOnChangeStatusIsRetried) {
    EXPECT_CALL(_orderRepository, findById(_)).WillOnce(Return(buildOrder(Order::OrderStatus::Created)));
    EXPECT_CALL(_statusRepository, changeStatus(_, _, _))
        .WillOnce(Return(std::unexpected(RepositoryError::ConnectionFailure)));

    EXPECT_FALSE(_handler.handle(validPayload(), validMetadata()));
}
