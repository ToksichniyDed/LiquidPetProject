//
// Created by DED on 25.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "NetworkConfiguration.h"

using namespace order_system::models;

namespace {
    struct NetworkConfigurationTestCase {
        std::string testName;
        nlohmann::json section;
        bool expectSuccess;
    };
}

class NetworkConfigurationFromJsonTest
        : public ::testing::TestWithParam<NetworkConfigurationTestCase> {
};

TEST_P(NetworkConfigurationFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result =
            order_system::models::NetworkConfiguration::fromJson(testCase.section);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->port, testCase.section.at("port").get<uint16_t>());
        EXPECT_EQ(result->address.value(), testCase.section.at("address").get<std::string>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    NetworkConfigurationTests,
    NetworkConfigurationFromJsonTest,
    ::testing::Values(
        NetworkConfigurationTestCase{
        .testName = "ValidAddressAndPort",
        .section = R"({"address": "0.0.0.0", "port": 8080})"_json,
        .expectSuccess = true
        },
        NetworkConfigurationTestCase{
        .testName = "MissingAddress",
        .section = R"({"port": 8080})"_json,
        .expectSuccess = false
        },
        NetworkConfigurationTestCase{
        .testName = "MissingPort",
        .section = R"({"address": "0.0.0.0"})"_json,
        .expectSuccess = false
        },
        NetworkConfigurationTestCase{
        .testName = "InvalidAddressFormat",
        .section = R"({"address": "not-an-ip", "port": 8080})"_json,
        .expectSuccess = false
        },
        NetworkConfigurationTestCase{
        .testName = "PortWrongType",
        .section = R"({"address": "0.0.0.0", "port": "8080"})"_json,
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<NetworkConfigurationTestCase>& info) {
    return info.param.testName;
    });
