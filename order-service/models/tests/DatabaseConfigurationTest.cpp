//
// Created by DED on 30.08.2026.
//

#include <gtest/gtest.h>

#include "DatabaseConfiguration.h"

using namespace order_system::models;

namespace {
    struct DatabaseConfigurationTestCase {
        std::string testName;
        std::string host;
        std::uint16_t port;
        std::string databaseName;
        std::string user;
        std::string password;
        bool useSsl;
        bool expectSuccess;
        DatabaseConfigurationError expectedError{};
    };
}

class DatabaseConfigurationCreateTest
        : public ::testing::TestWithParam<DatabaseConfigurationTestCase> {
};

TEST_P(DatabaseConfigurationCreateTest, Create) {
    const auto& testCase = GetParam();

    const auto result = DatabaseConfiguration::create(
        testCase.host, testCase.port, testCase.databaseName,
        testCase.user, testCase.password, testCase.useSsl
    );

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->host(), testCase.host);
        EXPECT_EQ(result->port(), testCase.port);
        EXPECT_EQ(result->databaseName(), testCase.databaseName);
        EXPECT_EQ(result->user(), testCase.user);
        EXPECT_EQ(result->useSsl(), testCase.useSsl);
        return;
    }

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), testCase.expectedError);
}

INSTANTIATE_TEST_SUITE_P(
    DatabaseConfigurationTests,
    DatabaseConfigurationCreateTest,
    ::testing::Values(
        DatabaseConfigurationTestCase{
        .testName = "ValidAllFields",
        .host = "localhost", .port = 5432, .databaseName = "orders",
        .user = "orders_user", .password = "secret", .useSsl = false,
        .expectSuccess = true
        },
        DatabaseConfigurationTestCase{
        .testName = "ValidWithSslEnabled",
        .host = "db.internal", .port = 5433, .databaseName = "analytics",
        .user = "reader", .password = "pw", .useSsl = true,
        .expectSuccess = true
        },
        DatabaseConfigurationTestCase{
        .testName = "ValidWithEmptyPassword",
        .host = "localhost", .port = 5432, .databaseName = "orders",
        .user = "orders_user", .password = "", .useSsl = false,
        .expectSuccess = true
        },
        DatabaseConfigurationTestCase{
        .testName = "EmptyHost",
        .host = "", .port = 5432, .databaseName = "orders",
        .user = "orders_user", .password = "secret", .useSsl = false,
        .expectSuccess = false, .expectedError = DatabaseConfigurationError::EmptyHost
        },
        DatabaseConfigurationTestCase{
        .testName = "EmptyDatabaseName",
        .host = "localhost", .port = 5432, .databaseName = "",
        .user = "orders_user", .password = "secret", .useSsl = false,
        .expectSuccess = false, .expectedError = DatabaseConfigurationError::EmptyDatabaseName
        },
        DatabaseConfigurationTestCase{
        .testName = "EmptyUser",
        .host = "localhost", .port = 5432, .databaseName = "orders",
        .user = "", .password = "secret", .useSsl = false,
        .expectSuccess = false, .expectedError = DatabaseConfigurationError::EmptyUser
        },
        DatabaseConfigurationTestCase{
        .testName = "ZeroPort",
        .host = "localhost", .port = 0, .databaseName = "orders",
        .user = "orders_user", .password = "secret", .useSsl = false,
        .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidPort
        }
    ),
    [](const ::testing::TestParamInfo<DatabaseConfigurationTestCase>& info) {
    return info.param.testName;
    });

class DatabaseConfigConnectionStringTest : public ::testing::Test {
protected:
    static DatabaseConfiguration makeConfig(bool useSsl) {
        return DatabaseConfiguration::create(
            "localhost", 5432, "orders", "orders_user", "secret", useSsl
        ).value();
    }
};

TEST_F(DatabaseConfigConnectionStringTest, ContainsAllFieldsWithSslDisabled) {
    std::string connectionString = makeConfig(false).toConnectionString();

    EXPECT_NE(connectionString.find("host=localhost"), std::string::npos);
    EXPECT_NE(connectionString.find("port=5432"), std::string::npos);
    EXPECT_NE(connectionString.find("dbname=orders"), std::string::npos);
    EXPECT_NE(connectionString.find("user=orders_user"), std::string::npos);
    EXPECT_NE(connectionString.find("password=secret"), std::string::npos);
    EXPECT_NE(connectionString.find("sslmode=disable"), std::string::npos);
}

TEST_F(DatabaseConfigConnectionStringTest, UsesSslmodeRequireWhenSslEnabled) {
    std::string connectionString = makeConfig(true).toConnectionString();

    EXPECT_NE(connectionString.find("sslmode=require"), std::string::npos);
    EXPECT_EQ(connectionString.find("sslmode=disable"), std::string::npos);
}

TEST(DatabaseConfigErrorCategoryTest, NameIsStable) {
    std::error_code ec = DatabaseConfigurationError::EmptyHost;

    EXPECT_STREQ(ec.category().name(), "databaseConfiguration");
}

TEST(DatabaseConfigErrorCategoryTest, MessagesAreHumanReadableForEachEnumValue) {
    EXPECT_EQ(std::error_code(DatabaseConfigurationError::EmptyHost).message(), "empty host");
    EXPECT_EQ(std::error_code(DatabaseConfigurationError::EmptyDatabaseName).message(), "empty database name");
    EXPECT_EQ(std::error_code(DatabaseConfigurationError::EmptyUser).message(), "empty user");
    EXPECT_EQ(std::error_code(DatabaseConfigurationError::InvalidPort).message(), "invalid port");
}
