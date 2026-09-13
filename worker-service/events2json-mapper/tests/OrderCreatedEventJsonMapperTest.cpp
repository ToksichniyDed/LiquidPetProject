//
// Created by DED on 13.09.2026.
//

#include <OrderCreatedEvent.h>
#include <gtest/gtest.h>
#include <mapper/OrderCreatedEventJsonMapper.h>

#include <nlohmann/json.hpp>

using namespace worker_service::events;

namespace {
constexpr auto VALID_ORDER_ID = "11111111-1111-1111-1111-111111111111";
constexpr auto VALID_USER_ID = "22222222-2222-2222-2222-222222222222";
constexpr auto VALID_PRODUCT_ID = "33333333-3333-3333-3333-333333333333";
constexpr auto OTHER_PRODUCT_ID = "44444444-4444-4444-4444-444444444444";

nlohmann::json validItemJson() { return {{"productId", VALID_PRODUCT_ID}, {"quantity", 3}}; }

nlohmann::json validEventJson() {
    return {
        {"orderId", VALID_ORDER_ID}, {"userId", VALID_USER_ID}, {"items", nlohmann::json::array({validItemJson()})}};
}
}  // namespace

struct OrderCreatedEventFromJsonTestCase {
    std::string testName;
    nlohmann::json json;
    bool expectSuccess;
};

class OrderCreatedEventJsonMapperFromJsonTest : public ::testing::TestWithParam<OrderCreatedEventFromJsonTestCase> {};

TEST_P(OrderCreatedEventJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = OrderCreatedEventJsonMapper::fromJson(testCase.json);

    EXPECT_EQ(result.has_value(), testCase.expectSuccess);
}

INSTANTIATE_TEST_SUITE_P(
    OrderCreatedEventJsonMapperTests, OrderCreatedEventJsonMapperFromJsonTest,
    ::testing::Values(OrderCreatedEventFromJsonTestCase{.testName = "ValidEventWithOneItem",
                                                        .json = validEventJson(),
                                                        .expectSuccess = true},
                      OrderCreatedEventFromJsonTestCase{.testName = "ValidEventWithMultipleItems",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                auto secondItem = validItemJson();
                                                                secondItem["productId"] = OTHER_PRODUCT_ID;
                                                                secondItem["quantity"] = 1;
                                                                j["items"].push_back(secondItem);
                                                                return j;
                                                            }(),
                                                        .expectSuccess = true},
                      OrderCreatedEventFromJsonTestCase{.testName = "MissingOrderId",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j.erase("orderId");
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "MissingUserId",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j.erase("userId");
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "MissingItems",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j.erase("items");
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "OrderIdNotUuid",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j["orderId"] = "not-a-uuid";
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "UserIdNotUuid",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j["userId"] = "not-a-uuid";
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "ItemsNotAnArray",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                j["items"] = "not-an-array";
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "ItemMissingProductId",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                auto badItem = validItemJson();
                                                                badItem.erase("productId");
                                                                j["items"] = nlohmann::json::array({badItem});
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "ItemMissingQuantity",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                auto badItem = validItemJson();
                                                                badItem.erase("quantity");
                                                                j["items"] = nlohmann::json::array({badItem});
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "ItemProductIdNotUuid",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                auto badItem = validItemJson();
                                                                badItem["productId"] = "not-a-uuid";
                                                                j["items"] = nlohmann::json::array({badItem});
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false},
                      OrderCreatedEventFromJsonTestCase{.testName = "OneOfMultipleItemsInvalid",
                                                        .json =
                                                            [] {
                                                                auto j = validEventJson();
                                                                auto badItem = validItemJson();
                                                                badItem["productId"] = "not-a-uuid";
                                                                j["items"].push_back(badItem);
                                                                return j;
                                                            }(),
                                                        .expectSuccess = false}),
    [](const ::testing::TestParamInfo<OrderCreatedEventFromJsonTestCase>& info) { return info.param.testName; });

TEST(OrderCreatedEventJsonMapperToJsonTest, ProducesExpectedStructure) {
    auto event = OrderCreatedEventJsonMapper::fromJson(validEventJson()).value();

    nlohmann::json json = OrderCreatedEventJsonMapper::toJson(event);

    EXPECT_EQ(json.at("orderId").get<std::string>(), VALID_ORDER_ID);
    EXPECT_EQ(json.at("userId").get<std::string>(), VALID_USER_ID);
    ASSERT_EQ(json.at("items").size(), 1);
    EXPECT_EQ(json.at("items")[0].at("productId").get<std::string>(), VALID_PRODUCT_ID);
    EXPECT_EQ(json.at("items")[0].at("quantity").get<int>(), 3);
}

TEST(OrderCreatedEventJsonMapperToJsonTest, ProducesEmptyItemsArrayWhenNoItems) {
    auto event = OrderCreatedEventJsonMapper::fromJson(validEventJson()).value();
    event.items.clear();

    nlohmann::json json = OrderCreatedEventJsonMapper::toJson(event);

    ASSERT_TRUE(json.at("items").is_array());
    EXPECT_TRUE(json.at("items").empty());
}

TEST(OrderCreatedEventJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesAllFields) {
    auto original = OrderCreatedEventJsonMapper::fromJson(validEventJson()).value();

    auto roundTripped = OrderCreatedEventJsonMapper::fromJson(OrderCreatedEventJsonMapper::toJson(original));

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped->orderId, original.orderId);
    EXPECT_EQ(roundTripped->userId, original.userId);
    ASSERT_EQ(roundTripped->items.size(), original.items.size());
    EXPECT_EQ(roundTripped->items[0].productId, original.items[0].productId);
    EXPECT_EQ(roundTripped->items[0].quantity, original.items[0].quantity);
}
