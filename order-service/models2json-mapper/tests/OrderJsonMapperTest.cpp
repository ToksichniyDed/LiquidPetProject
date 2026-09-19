//
// Created by DED on 30.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <Order.h>
#include <mapper/OrderJsonMapper.h>

using namespace order_system::models;
using namespace order_system::models2json_mapper;

namespace {
    constexpr auto VALID_USER_ID = "11111111-1111-1111-1111-111111111111";
    constexpr auto VALID_PRODUCT_ID = "22222222-2222-2222-2222-222222222222";

    nlohmann::json validItemJson() {
        return {
            {"productId", VALID_PRODUCT_ID},
            {"quantity", 3},
            {
                "priceAtOrderTime", {
                    {"minorUnits", 500},
                    {"currency", {{"code", "USD"}, {"minorDigits", 2}}}
                }
            }
        };
    }

    nlohmann::json validOrderJson() {
        return {
            {"userId", VALID_USER_ID},
            {"items", nlohmann::json::array({validItemJson()})}
        };
    }
}

struct OrderItemFromJsonTestCase {
    std::string testName;
    nlohmann::json json;
    bool expectSuccess;
};

class OrderItemJsonMapperFromJsonTest
        : public ::testing::TestWithParam<OrderItemFromJsonTestCase> {
};

TEST_P(OrderItemJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = OrderItemJsonMapper::fromJson(testCase.json);

    EXPECT_EQ(result.has_value(), testCase.expectSuccess);
}

INSTANTIATE_TEST_SUITE_P(
    OrderItemJsonMapperTests,
    OrderItemJsonMapperFromJsonTest,
    ::testing::Values(
        OrderItemFromJsonTestCase{
        .testName = "ValidItem",
        .json = validItemJson(),
        .expectSuccess = true
        },
        OrderItemFromJsonTestCase{
        .testName = "MissingProductId",
        .json = []{ auto j = validItemJson(); j.erase("productId"); return j; }(),
        .expectSuccess = false
        },
        OrderItemFromJsonTestCase{
        .testName = "MissingQuantity",
        .json = []{ auto j = validItemJson(); j.erase("quantity"); return j; }(),
        .expectSuccess = false
        },
        OrderItemFromJsonTestCase{
        .testName = "MissingPrice",
        .json = []{ auto j = validItemJson(); j.erase("priceAtOrderTime"); return j; }(),
        .expectSuccess = false
        },
        OrderItemFromJsonTestCase{
        .testName = "ZeroQuantity",
        .json = []{ auto j = validItemJson(); j["quantity"] = 0; return j; }(),
        .expectSuccess = false
        },
        OrderItemFromJsonTestCase{
        .testName = "ProductIdNotUuid",
        .json = []{ auto j = validItemJson(); j["productId"] = "not-a-uuid"; return j; }(),
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<OrderItemFromJsonTestCase>& info) {
    return info.param.testName;
    });

TEST(OrderItemJsonMapperToJsonTest, ProducesExpectedStructure) {
    auto item = OrderItemJsonMapper::fromJson(validItemJson()).value();

    nlohmann::json json = OrderItemJsonMapper::toJson(item);

    EXPECT_EQ(json.at("productId").get<std::string>(), VALID_PRODUCT_ID);
    EXPECT_EQ(json.at("quantity").get<int>(), 3);
    EXPECT_EQ(json.at("priceAtOrderTime").at("minorUnits").get<std::int64_t>(), 500);
}

TEST(OrderItemJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesValue) {
    auto original = OrderItemJsonMapper::fromJson(validItemJson()).value();

    auto roundTripped = OrderItemJsonMapper::fromJson(OrderItemJsonMapper::toJson(original));

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped->productId(), original.productId());
    EXPECT_EQ(roundTripped->quantity(), original.quantity());
    EXPECT_EQ(roundTripped->priceAtOrderTime(), original.priceAtOrderTime());
}

struct OrderFromJsonTestCase {
    std::string testName;
    nlohmann::json json;
    bool expectSuccess;
};

class OrderJsonMapperFromJsonTest
        : public ::testing::TestWithParam<OrderFromJsonTestCase> {
};

TEST_P(OrderJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = OrderJsonMapper::fromJson(testCase.json);

    EXPECT_EQ(result.has_value(), testCase.expectSuccess);
}

INSTANTIATE_TEST_SUITE_P(
    OrderJsonMapperTests,
    OrderJsonMapperFromJsonTest,
    ::testing::Values(
        OrderFromJsonTestCase{
        .testName = "ValidOrderWithOneItem",
        .json = validOrderJson(),
        .expectSuccess = true
        },
        OrderFromJsonTestCase{
        .testName = "MissingUserId",
        .json = []{ auto j = validOrderJson(); j.erase("userId"); return j; }(),
        .expectSuccess = false
        },
        OrderFromJsonTestCase{
        .testName = "MissingItems",
        .json = []{ auto j = validOrderJson(); j.erase("items"); return j; }(),
        .expectSuccess = false
        },
        OrderFromJsonTestCase{
        .testName = "EmptyItemsArray",
        .json = []{ auto j = validOrderJson(); j["items"] = nlohmann::json::array(); return j; }(),
        .expectSuccess = false
        },
        OrderFromJsonTestCase{
        .testName = "ItemsNotAnArray",
        .json = []{ auto j = validOrderJson(); j["items"] = "not-an-array"; return j; }(),
        .expectSuccess = false
        },
        OrderFromJsonTestCase{
        .testName = "OneOfMultipleItemsInvalid",
        .json = []{
        auto j = validOrderJson();
        auto badItem = validItemJson();
        badItem["quantity"] = -1;
        j["items"].push_back(badItem);
        return j;
        }(),
        .expectSuccess = false
        },
        OrderFromJsonTestCase{
        .testName = "UserIdNotUuid",
        .json = []{ auto j = validOrderJson(); j["userId"] = "not-a-uuid"; return j; }(),
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<OrderFromJsonTestCase>& info) {
    return info.param.testName;
    });

TEST(OrderJsonMapperToJsonTest, OmitsOrderIdWhenNotAssigned) {
    auto order = OrderJsonMapper::fromJson(validOrderJson()).value();

    nlohmann::json json = OrderJsonMapper::toJson(order);

    EXPECT_FALSE(json.contains("orderId"));
    EXPECT_EQ(json.at("userId").get<std::string>(), VALID_USER_ID);
    EXPECT_EQ(json.at("items").size(), 1);
}

TEST(OrderJsonMapperToJsonTest, IncludesOrderIdWhenAssigned) {
    auto order = OrderJsonMapper::fromJson(validOrderJson()).value();
    order.assignId(OrderId::create("33333333-3333-3333-3333-333333333333").value());

    nlohmann::json json = OrderJsonMapper::toJson(order);

    ASSERT_TRUE(json.contains("orderId"));
    EXPECT_EQ(json.at("orderId").get<std::string>(), "33333333-3333-3333-3333-333333333333");
}

TEST(OrderJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesItemsAndUserId) {
    auto original = OrderJsonMapper::fromJson(validOrderJson()).value();

    auto roundTripped = OrderJsonMapper::fromJson(OrderJsonMapper::toJson(original));

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped->userId(), original.userId());
    ASSERT_EQ(roundTripped->items().size(), original.items().size());
    EXPECT_EQ(roundTripped->items()[0].productId(), original.items()[0].productId());
}
