//
// Created by DED on 30.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <mapper/CurrencyJsonMapper.h>

using namespace order_system::models;
using namespace order_system::models2json_mapper;

namespace {
    struct CurrencyFromJsonTestCase {
        std::string testName;
        nlohmann::json json;
        bool expectSuccess;
    };
}

class CurrencyJsonMapperFromJsonTest
        : public ::testing::TestWithParam<CurrencyFromJsonTestCase> {
};

TEST_P(CurrencyJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = CurrencyJsonMapper::fromJson(testCase.json);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->code(), testCase.json.at("code").get<std::string>());
        EXPECT_EQ(result->minorDigits(), testCase.json.at("minorDigits").get<std::int8_t>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    CurrencyJsonMapperTests,
    CurrencyJsonMapperFromJsonTest,
    ::testing::Values(
        CurrencyFromJsonTestCase{
        .testName = "ValidCodeAndDigits",
        .json = R"({"code": "USD", "minorDigits": 2})"_json,
        .expectSuccess = true
        },
        CurrencyFromJsonTestCase{
        .testName = "MissingCode",
        .json = R"({"minorDigits": 2})"_json,
        .expectSuccess = false
        },
        CurrencyFromJsonTestCase{
        .testName = "MissingMinorDigits",
        .json = R"({"code": "USD"})"_json,
        .expectSuccess = false
        },
        CurrencyFromJsonTestCase{
        .testName = "EmptyCode",
        .json = R"({"code": "", "minorDigits": 2})"_json,
        .expectSuccess = false
        },
        CurrencyFromJsonTestCase{
        .testName = "NegativeMinorDigits",
        .json = R"({"code": "USD", "minorDigits": -1})"_json,
        .expectSuccess = false
        },
        CurrencyFromJsonTestCase{
        .testName = "MinorDigitsWrongType",
        .json = R"({"code": "USD", "minorDigits": "two"})"_json,
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<CurrencyFromJsonTestCase>& info) {
    return info.param.testName;
    });

TEST(CurrencyJsonMapperToJsonTest, ProducesExpectedKeys) {
    auto currency = Currency::create("EUR", 2).value();

    nlohmann::json json = CurrencyJsonMapper::toJson(currency);

    EXPECT_EQ(json.at("code").get<std::string>(), "EUR");
    EXPECT_EQ(json.at("minorDigits").get<std::int8_t>(), 2);
}

TEST(CurrencyJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesValue) {
    auto original = Currency::create("JPY", 0).value();

    auto roundTripped = CurrencyJsonMapper::fromJson(CurrencyJsonMapper::toJson(original));

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped.value(), original);
}
