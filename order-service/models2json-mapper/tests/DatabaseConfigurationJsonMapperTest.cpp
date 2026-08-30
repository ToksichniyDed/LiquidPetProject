//
// Created by DED on 31.08.2026.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <mapper/DatabaseConfigurationJsonMapper.h>
#include "DatabaseConfiguration.h"

using namespace order_system::models2json_mapper;

namespace {
    nlohmann::json validDatabaseJson() {
        return {
            {"host", "postgres"},
            {"port", 5432},
            {"databaseName", "orders"},
            {"user", "orders_user"},
            {"useSsl", false}
        };
    }

    struct DatabaseConfigFromJsonTestCase {
        std::string testName;
        nlohmann::json json;
        std::string password;
        bool expectSuccess;
    };
}

class DatabaseConfigurationJsonMapperFromJsonTest
        : public ::testing::TestWithParam<DatabaseConfigFromJsonTestCase> {
};

TEST_P(DatabaseConfigurationJsonMapperFromJsonTest, FromJson) {
    const auto& testCase = GetParam();

    const auto result = DatabaseConfigurationJsonMapper::fromJson(testCase.json, testCase.password);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->host(), testCase.json.at("host").get<std::string>());
        EXPECT_EQ(result->port(), testCase.json.at("port").get<std::uint16_t>());
        EXPECT_EQ(result->databaseName(), testCase.json.at("databaseName").get<std::string>());
        EXPECT_EQ(result->user(), testCase.json.at("user").get<std::string>());
        EXPECT_EQ(result->useSsl(), testCase.json.at("useSsl").get<bool>());
        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    DatabaseConfigurationJsonMapperTests,
    DatabaseConfigurationJsonMapperFromJsonTest,
    ::testing::Values(
        DatabaseConfigFromJsonTestCase{
        .testName = "ValidAllFields",
        .json = validDatabaseJson(),
        .password = "secret",
        .expectSuccess = true
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "ValidWithSslEnabled",
        .json = []{ auto j = validDatabaseJson(); j["useSsl"] = true; return j; }(),
        .password = "secret",
        .expectSuccess = true
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "ValidWithEmptyPassword",
        .json = validDatabaseJson(),
        .password = "",
        .expectSuccess = true
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "MissingHost",
        .json = []{ auto j = validDatabaseJson(); j.erase("host"); return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "MissingPort",
        .json = []{ auto j = validDatabaseJson(); j.erase("port"); return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "MissingDatabaseName",
        .json = []{ auto j = validDatabaseJson(); j.erase("databaseName"); return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "MissingUser",
        .json = []{ auto j = validDatabaseJson(); j.erase("user"); return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "MissingUseSsl",
        .json = []{ auto j = validDatabaseJson(); j.erase("useSsl"); return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "EmptyHost",
        .json = []{ auto j = validDatabaseJson(); j["host"] = ""; return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "ZeroPort",
        .json = []{ auto j = validDatabaseJson(); j["port"] = 0; return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "PortWrongType",
        .json = []{ auto j = validDatabaseJson(); j["port"] = "5432"; return j; }(),
        .password = "secret",
        .expectSuccess = false
        },
        DatabaseConfigFromJsonTestCase{
        .testName = "UseSslWrongType",
        .json = []{ auto j = validDatabaseJson(); j["useSsl"] = "true"; return j; }(),
        .password = "secret",
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<DatabaseConfigFromJsonTestCase>& info) {
    return info.param.testName;
    });

class DatabaseConfigurationJsonMapperToJsonTest : public ::testing::Test {
protected:
    static order_system::models::DatabaseConfiguration makeConfig() {
        return DatabaseConfigurationJsonMapper::fromJson(validDatabaseJson(), "super-secret-password").value();
    }
};

TEST_F(DatabaseConfigurationJsonMapperToJsonTest, ProducesExpectedNonSecretFields) {
    nlohmann::json json = DatabaseConfigurationJsonMapper::toJson(makeConfig());

    EXPECT_EQ(json.at("host").get<std::string>(), "postgres");
    EXPECT_EQ(json.at("port").get<std::uint16_t>(), 5432);
    EXPECT_EQ(json.at("databaseName").get<std::string>(), "orders");
    EXPECT_EQ(json.at("user").get<std::string>(), "orders_user");
    EXPECT_EQ(json.at("useSsl").get<bool>(), false);
}

TEST_F(DatabaseConfigurationJsonMapperToJsonTest, NeverIncludesPasswordKey) {
    nlohmann::json json = DatabaseConfigurationJsonMapper::toJson(makeConfig());

    EXPECT_FALSE(json.contains("password"));
}

TEST_F(DatabaseConfigurationJsonMapperToJsonTest, SerializedJsonDoesNotContainPasswordValueAnywhere) {
    nlohmann::json json = DatabaseConfigurationJsonMapper::toJson(makeConfig());

    EXPECT_EQ(json.dump().find("super-secret-password"), std::string::npos);
}

TEST(DatabaseConfigurationJsonMapperRoundTripTest, ToJsonThenFromJsonPreservesNonSecretFields) {
    auto original = DatabaseConfigurationJsonMapper::fromJson(validDatabaseJson(), "secret").value();

    auto roundTripped = DatabaseConfigurationJsonMapper::fromJson(
        DatabaseConfigurationJsonMapper::toJson(original), "secret"
    );

    ASSERT_TRUE(roundTripped.has_value());
    EXPECT_EQ(roundTripped->host(), original.host());
    EXPECT_EQ(roundTripped->port(), original.port());
    EXPECT_EQ(roundTripped->databaseName(), original.databaseName());
    EXPECT_EQ(roundTripped->user(), original.user());
    EXPECT_EQ(roundTripped->useSsl(), original.useSsl());
}
