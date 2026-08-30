//
// Created by DED on 30.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <mapper/MoneyJsonMapper.h>

using namespace order_system::models;
using namespace order_system::models2json_mapper;

namespace {
    struct MoneyFromJsonTestCase {
        std::string testName;
        nlohmann::json json;
        bool expectSuccess;
    };
}

class MoneyJsonMapperFromJsonTest
        : public ::testing::TestWithParam<MoneyFromJsonTestCase> {
};

TEST_P(MoneyJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = MoneyJsonMapper::fromJson(testCase.json);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->minorUnits(), testCase.json.at("minorUnits").get<std::int64_t>());
        EXPECT_EQ(result->currency().code(), testCase.json.at("currency").at("code").get<std::string>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    MoneyJsonMapperTests,
    MoneyJsonMapperFromJsonTest,
    ::testing::Values(
        MoneyFromJsonTestCase{
        .testName = "ValidMinorUnitsAndCurrency",
        .json = R"({"minorUnits": 1999, "currency": {"code": "USD", "minorDigits": 2}})"_json,
        .expectSuccess = true
        },
        MoneyFromJsonTestCase{
        .testName = "MissingMinorUnits",
        .json = R"({"currency": {"code": "USD", "minorDigits": 2}})"_json,
        .expectSuccess = false
        },
        MoneyFromJsonTestCase{
        .testName = "MissingCurrency",
        .json = R"({"minorUnits": 1999})"_json,
        .expectSuccess = false
        },
        MoneyFromJsonTestCase{
        .testName = "NegativeMinorUnits",
        .json = R"({"minorUnits": -5, "currency": {"code": "USD", "minorDigits": 2}})"_json,
        .expectSuccess = false
        },
        MoneyFromJsonTestCase{
        .testName = "InvalidNestedCurrency",
        // currency сам по себе невалиден (пустой code) — ошибка должна
        // всплыть через CurrencyJsonMapper::fromJson внутри MoneyJsonMapper::fromJson
        .json = R"({"minorUnits": 1999, "currency": {"code": "", "minorDigits": 2}})"_json,
        .expectSuccess = false
        },
        MoneyFromJsonTestCase{
        .testName = "CurrencyWrongType",
        .json = R"({"minorUnits": 1999, "currency": "USD"})"_json,
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<MoneyFromJsonTestCase>& info) {
    return info.param.testName;
    });

TEST(MoneyJsonMapperToJsonTest, ProducesExpectedStructure) {
    auto currency = Currency::create("USD", 2).value();
    auto money = Money::create(1999, currency).value();

    nlohmann::json json = MoneyJsonMapper::toJson(money);

    EXPECT_EQ(json.at("minorUnits").get<std::int64_t>(), 1999);
    EXPECT_EQ(json.at("currency").at("code").get<std::string>(), "USD");
    EXPECT_EQ(json.at("currency").at("minorDigits").get<std::int8_t>(), 2);
}

TEST(MoneyJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesValue) {
    auto currency = Currency::create("GBP", 2).value();
    auto original = Money::create(4250, currency).value();

    auto roundTripped = MoneyJsonMapper::fromJson(MoneyJsonMapper::toJson(original));

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped.value(), original);
}
