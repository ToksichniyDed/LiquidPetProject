//
// Created by DED on 16.09.2026.
//

#include <gtest/gtest.h>

#include <cstdlib>

#include "EnvironmentConfiguration.h"

using namespace shared::models;

namespace {
class ScopedEnvVar {
   public:
    ScopedEnvVar(std::string name, const std::string& value) : _name(std::move(name)) {
        setenv(_name.c_str(), value.c_str(), 1);
    }

    ~ScopedEnvVar() { unsetenv(_name.c_str()); }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

   private:
    std::string _name;
};

}  // namespace

class EnvironmentConfigurationTest : public ::testing::Test {
   protected:
    void TearDown() override {
        // подчищаем на случай, если тест сам не воспользовался ScopedEnvVar
        unsetenv("ENV_CONFIG_TEST_REQUIRED");
        unsetenv("ENV_CONFIG_TEST_OPTIONAL");
        unsetenv("ENV_CONFIG_TEST_WITH_DEFAULT");
    }
};

TEST_F(EnvironmentConfigurationTest, LoadSucceedsWhenRequiredVariableIsPresent) {
    ScopedEnvVar var("ENV_CONFIG_TEST_REQUIRED", "some-value");

    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_REQUIRED", .required = true},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("ENV_CONFIG_TEST_REQUIRED"), "some-value");
}

TEST_F(EnvironmentConfigurationTest, LoadFailsWhenRequiredVariableIsMissing) {
    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_REQUIRED", .required = true},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EnvironmentConfigurationError::MissingRequiredVariable);
}

TEST_F(EnvironmentConfigurationTest, LoadSucceedsWhenOptionalVariableIsMissing) {
    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_OPTIONAL", .required = false},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("ENV_CONFIG_TEST_OPTIONAL"), std::nullopt);
}

TEST_F(EnvironmentConfigurationTest, LoadUsesDefaultValueWhenVariableIsMissing) {
    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_WITH_DEFAULT", .required = false, .defaultValue = "default-value"},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("ENV_CONFIG_TEST_WITH_DEFAULT"), "default-value");
}

TEST_F(EnvironmentConfigurationTest, LoadPrefersExplicitValueOverDefault) {
    ScopedEnvVar var("ENV_CONFIG_TEST_WITH_DEFAULT", "explicit-value");

    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_WITH_DEFAULT", .required = false, .defaultValue = "default-value"},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("ENV_CONFIG_TEST_WITH_DEFAULT"), "explicit-value");
}

TEST_F(EnvironmentConfigurationTest, RequiredFlagIsIgnoredWhenDefaultValueIsProvided) {
    // required=true, но есть defaultValue - отсутствие переменной не должно
    // приводить к ошибке, т.к. значение всё равно определено
    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_WITH_DEFAULT", .required = true, .defaultValue = "fallback"},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("ENV_CONFIG_TEST_WITH_DEFAULT"), "fallback");
}

TEST_F(EnvironmentConfigurationTest, LoadFailsOnFirstMissingRequiredVariableAmongMultiple) {
    ScopedEnvVar present("ENV_CONFIG_TEST_OPTIONAL", "present");

    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_OPTIONAL", .required = true},
        {.name = "ENV_CONFIG_TEST_REQUIRED", .required = true},
    };

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EnvironmentConfigurationError::MissingRequiredVariable);
}

TEST_F(EnvironmentConfigurationTest, EmptySchemaLoadsSuccessfully) {
    EnvironmentSchema schema = {};

    auto result = EnvironmentConfiguration::load(schema);

    ASSERT_TRUE(result.has_value());
}

TEST_F(EnvironmentConfigurationTest, GetReturnsNulloptForUnknownKey) {
    EnvironmentSchema schema = {};

    auto result = EnvironmentConfiguration::load(schema);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->get("SOME_KEY_NOT_IN_SCHEMA"), std::nullopt);
}

TEST_F(EnvironmentConfigurationTest, RequireReturnsValueForPresentVariable) {
    ScopedEnvVar var("ENV_CONFIG_TEST_REQUIRED", "required-value");

    EnvironmentSchema schema = {
        {.name = "ENV_CONFIG_TEST_REQUIRED", .required = true},
    };

    auto result = EnvironmentConfiguration::load(schema);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->require("ENV_CONFIG_TEST_REQUIRED"), "required-value");
}

TEST_F(EnvironmentConfigurationTest, RequireThrowsForVariableNotInSchema) {
    EnvironmentSchema schema = {};

    auto result = EnvironmentConfiguration::load(schema);
    ASSERT_TRUE(result.has_value());

    // require() - это std::unordered_map::at(), поэтому для
    // ключа, которого нет в _values, ожидаем std::out_of_range.
    EXPECT_THROW(result->require("MISSING_KEY"), std::out_of_range);
}

TEST_F(EnvironmentConfigurationTest, ErrorCategoryNameIsStable) {
    std::error_code ec = EnvironmentConfigurationError::MissingRequiredVariable;

    EXPECT_STREQ(ec.category().name(), "environmentConfiguration");
}

TEST_F(EnvironmentConfigurationTest, ErrorMessageIsHumanReadable) {
    EXPECT_EQ(std::error_code(EnvironmentConfigurationError::MissingRequiredVariable).message(),
              "missing required environment variable");
}
