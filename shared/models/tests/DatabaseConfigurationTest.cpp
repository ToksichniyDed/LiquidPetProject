//
// Created by DED on 30.08.2026.
//

#include <gtest/gtest.h>

#include "DatabaseConfiguration.h"

using namespace shared::models;

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

namespace {
    struct DatabaseConfigurationFromUrlTestCase {
        std::string testName;
        std::string url;
        bool expectSuccess;
        std::string expectedHost;
        std::uint16_t expectedPort{};
        std::string expectedDatabaseName;
        std::string expectedUser;
        bool expectedUseSsl{};
        DatabaseConfigurationError expectedError{};
    };
}

class DatabaseConfigurationFromUrlTest
        : public ::testing::TestWithParam<DatabaseConfigurationFromUrlTestCase> {
};

TEST_P(DatabaseConfigurationFromUrlTest, FromUrl) {
    const auto& testCase = GetParam();

    const auto result = DatabaseConfiguration::fromUrl(testCase.url);

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->host(), testCase.expectedHost);
        EXPECT_EQ(result->port(), testCase.expectedPort);
        EXPECT_EQ(result->databaseName(), testCase.expectedDatabaseName);
        EXPECT_EQ(result->user(), testCase.expectedUser);
        EXPECT_EQ(result->useSsl(), testCase.expectedUseSsl);
        return;
    }

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), testCase.expectedError);
}

INSTANTIATE_TEST_SUITE_P(
    DatabaseConfigurationFromUrlTests,
    DatabaseConfigurationFromUrlTest,
    ::testing::Values(
        DatabaseConfigurationFromUrlTestCase{
            .testName = "ValidUrlWithSslDisabled",
            .url = "postgres://orders:secret@localhost:5432/orders?sslmode=disable",
            .expectSuccess = true,
            .expectedHost = "localhost", .expectedPort = 5432,
            .expectedDatabaseName = "orders", .expectedUser = "orders",
            .expectedUseSsl = false
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "ValidUrlWithSslRequire",
            .url = "postgres://reader:pw@db.internal:5433/analytics?sslmode=require",
            .expectSuccess = true,
            .expectedHost = "db.internal", .expectedPort = 5433,
            .expectedDatabaseName = "analytics", .expectedUser = "reader",
            .expectedUseSsl = true
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "ValidUrlWithoutSslmodeParam",
            .url = "postgres://orders:secret@localhost:5432/orders",
            .expectSuccess = true,
            .expectedHost = "localhost", .expectedPort = 5432,
            .expectedDatabaseName = "orders", .expectedUser = "orders",
            .expectedUseSsl = false
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "ValidUrlWithPostgresqlScheme",
            .url = "postgresql://orders:secret@localhost:5432/orders?sslmode=disable",
            .expectSuccess = true,
            .expectedHost = "localhost", .expectedPort = 5432,
            .expectedDatabaseName = "orders", .expectedUser = "orders",
            .expectedUseSsl = false
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "InvalidScheme",
            .url = "mysql://orders:secret@localhost:5432/orders",
            .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidUrl
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "MissingPort",
            .url = "postgres://orders:secret@localhost/orders",
            .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidUrl
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "MissingDatabaseName",
            .url = "postgres://orders:secret@localhost:5432/",
            .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidUrl
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "EmptyString",
            .url = "",
            .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidUrl
        },
        DatabaseConfigurationFromUrlTestCase{
            .testName = "NotAUrlAtAll",
            .url = "just some random text",
            .expectSuccess = false, .expectedError = DatabaseConfigurationError::InvalidUrl
        }
    ),
    [](const ::testing::TestParamInfo<DatabaseConfigurationFromUrlTestCase>& info) {
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
    EXPECT_EQ(std::error_code(DatabaseConfigurationError::InvalidUrl).message(), "invalid url");
}
