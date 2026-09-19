//
// Created by DED on 19.09.2026.
//

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "OrderReservedEventJsonMapper.h"

using namespace order_service::event_handlers;

namespace {
    struct OrderReservedEventFromJsonTestCase {
        std::string testName;
        nlohmann::json json;
        bool expectSuccess;
    };
}  // namespace

class OrderReservedEventJsonMapperFromJsonTest
    : public ::testing::TestWithParam<OrderReservedEventFromJsonTestCase> {};

TEST_P(OrderReservedEventJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    EXPECT_EQ(OrderReservedEventJsonMapper::fromJson(testCase.json).has_value(), testCase.expectSuccess);
}

INSTANTIATE_TEST_SUITE_P(
    OrderReservedEventJsonMapperTests, OrderReservedEventJsonMapperFromJsonTest,
    ::testing::Values(
        OrderReservedEventFromJsonTestCase{
            .testName = "ValidOrderId",
            .json = R"({"orderId": "11111111-1111-1111-1111-111111111111"})"_json,
            .expectSuccess = true},
        OrderReservedEventFromJsonTestCase{
            .testName = "ExtraFieldsAreIgnored",
            .json = R"({"orderId": "11111111-1111-1111-1111-111111111111", "extra": 1})"_json,
            .expectSuccess = true},
        OrderReservedEventFromJsonTestCase{.testName = "MissingOrderId", .json = R"({})"_json, .expectSuccess = false},
        OrderReservedEventFromJsonTestCase{
            .testName = "OrderIdNotUuid", .json = R"({"orderId": "not-a-uuid"})"_json, .expectSuccess = false},
        OrderReservedEventFromJsonTestCase{
            .testName = "OrderIdWrongType", .json = R"({"orderId": 123})"_json, .expectSuccess = false}),
    [](const ::testing::TestParamInfo<OrderReservedEventFromJsonTestCase>& info) { return info.param.testName; });
