//
// Created by DED on 13.09.2026.
//

#include <OrderReservedEvent.h>
#include <gtest/gtest.h>
#include <mapper/OrderReservedEventJsonMapper.h>
#include <models/OrderIds.h>

#include <nlohmann/json.hpp>

using namespace worker_service::events;

namespace {
constexpr auto VALID_ORDER_ID = "11111111-1111-1111-1111-111111111111";
}

TEST(OrderReservedEventJsonMapperToJsonTest, ProducesOrderIdKey) {
    const OrderReservedEvent event{.orderId = shared::models::OrderId::create(VALID_ORDER_ID).value()};

    nlohmann::json json = OrderReservedEventJsonMapper::toJson(event);

    ASSERT_TRUE(json.contains("orderId"));
    EXPECT_EQ(json.at("orderId").get<std::string>(), VALID_ORDER_ID);
}

TEST(OrderReservedEventJsonMapperToJsonTest, ProducesOnlyOrderIdKeyAndNothingElse) {
    const OrderReservedEvent event{.orderId = shared::models::OrderId::create(VALID_ORDER_ID).value()};

    nlohmann::json json = OrderReservedEventJsonMapper::toJson(event);

    EXPECT_EQ(json.size(), 1);
}

TEST(OrderReservedEventJsonMapperToJsonTest, DifferentOrderIdsProduceDifferentJson) {
    const OrderReservedEvent firstEvent{
        .orderId = shared::models::OrderId::create("11111111-1111-1111-1111-111111111111").value()};
    const OrderReservedEvent secondEvent{
        .orderId = shared::models::OrderId::create("22222222-2222-2222-2222-222222222222").value()};

    nlohmann::json firstJson = OrderReservedEventJsonMapper::toJson(firstEvent);
    nlohmann::json secondJson = OrderReservedEventJsonMapper::toJson(secondEvent);

    EXPECT_NE(firstJson.at("orderId").get<std::string>(), secondJson.at("orderId").get<std::string>());
}
