//
// Created by DED on 17.09.2026.
//

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "mapper/UpstreamConfigurationJsonMapper.h"

namespace {
    struct UpstreamConfigurationTestCase {
        std::string testName;
        nlohmann::json section;
        bool expectSuccess;
    };
}

class UpstreamConfigurationJsonMapperFromJsonTest
        : public ::testing::TestWithParam<UpstreamConfigurationTestCase> {
};

TEST_P(UpstreamConfigurationJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = shared::models2json_mapper::UpstreamConfigurationJsonMapper::fromJson(testCase.section);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->port, testCase.section.at("port").get<uint16_t>());
        EXPECT_EQ(result->host.value(), testCase.section.at("address").get<std::string>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    UpstreamConfigurationJsonMapperTests,
    UpstreamConfigurationJsonMapperFromJsonTest,
    ::testing::Values(
        UpstreamConfigurationTestCase{
            .testName = "ValidIpLiteralAddressAndPort",
            .section = R"({"address": "127.0.0.1", "port": 8080})"_json,
            .expectSuccess = true
        },
        UpstreamConfigurationTestCase{
            .testName = "ValidDockerServiceNameAndPort",
            .section = R"({"address": "order-service", "port": 8080})"_json,
            .expectSuccess = true
        },
        UpstreamConfigurationTestCase{
            .testName = "MissingAddress",
            .section = R"({"port": 8080})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "MissingPort",
            .section = R"({"address": "order-service"})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "InvalidAddressFormat",
            .section = R"({"address": "order_service", "port": 8080})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "EmptyAddress",
            .section = R"({"address": "", "port": 8080})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "PortWrongType",
            .section = R"({"address": "order-service", "port": "8080"})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "AddressWrongType",
            .section = R"({"address": 12345, "port": 8080})"_json,
            .expectSuccess = false
        },
        UpstreamConfigurationTestCase{
            .testName = "EmptyObject",
            .section = R"({})"_json,
            .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<UpstreamConfigurationTestCase>& info) {
        return info.param.testName;
    });
