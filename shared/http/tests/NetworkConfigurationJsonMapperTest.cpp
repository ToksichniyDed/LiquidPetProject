//
// Created by DED on 31.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../NetworkConfigurationJsonMapper.h"
#include "../NetworkConfiguration.h"


namespace {
    struct NetworkConfigurationTestCase {
        std::string testName;
        nlohmann::json section;
        bool expectSuccess;
    };
}

class NetworkConfigurationJsonMapperFromJsonTest
        : public ::testing::TestWithParam<NetworkConfigurationTestCase> {
};

TEST_P(NetworkConfigurationJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(testCase.section);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->port, testCase.section.at("port").get<uint16_t>());
        EXPECT_EQ(result->address.value(), testCase.section.at("address").get<std::string>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    NetworkConfigurationJsonMapperTests,
    NetworkConfigurationJsonMapperFromJsonTest,
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
        },
        NetworkConfigurationTestCase{
        .testName = "AddressWrongType",
        .section = R"({"address": 12345, "port": 8080})"_json,
        .expectSuccess = false
        },
        NetworkConfigurationTestCase{
        .testName = "EmptyObject",
        .section = R"({})"_json,
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<NetworkConfigurationTestCase>& info) {
    return info.param.testName;
    });
