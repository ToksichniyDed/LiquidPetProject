//
// Created by DED on 23.08.2026.
//

#include <fstream>
#include <gtest/gtest.h>
#include <json/Json.h>

namespace {
    struct LoadSectionTestCase {
        std::string testName;
        std::optional<std::string> fileContent;
        std::string sectionName;
        bool expectSuccess;
        std::optional<Json::JsonParseError> expectedError;
    };
}

class LoadSectionTest : public ::testing::TestWithParam<LoadSectionTestCase> {
protected:
    void SetUp() override {
        testFilePath =
                std::filesystem::temp_directory_path()
                / std::format(
                    "configuration_loader_test_{}.json",
                    GetParam().testName
                );

        if (GetParam().fileContent.has_value()) {
            std::ofstream file(testFilePath);

            ASSERT_TRUE(file.is_open());

            file << *GetParam().fileContent;

            ASSERT_TRUE(file.good());
        }
    }

    void TearDown() override {
        std::filesystem::remove(testFilePath);
    }

protected:
    std::filesystem::path testFilePath;
};

TEST_P(LoadSectionTest, LoadSection) {
    const auto& testCase = GetParam();

    const auto result =
            Json::JsonHelper::loadSection(
                testFilePath,
                testCase.sectionName
            );

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());

        return;
    }

    ASSERT_FALSE(result.has_value());

    if (testCase.expectedError.has_value()) {
        EXPECT_EQ(
            result.error(),
            *testCase.expectedError
        );
    }
}

INSTANTIATE_TEST_SUITE_P(LoadSectionTests,
                         LoadSectionTest,
                         ::testing::Values(
                             LoadSectionTestCase{
                             .testName = "LoadSectionSuccess",
                             .fileContent = R"({"network": {"address": "0.0.0.0", "port": 8080}})",
                             .sectionName = "network",
                             .expectSuccess = true,
                             .expectedError = std::nullopt
                             },
                             LoadSectionTestCase{
                             .testName = "LoadSectionInvalidPathError",
                             .fileContent = std::nullopt,
                             .sectionName = {},
                             .expectSuccess = false,
                             .expectedError = Json::JsonParseError::InvalidPath

                             },
                             LoadSectionTestCase{
                             .testName = "LoadSectionInvalidFormatError",
                             .fileContent = R"({"network": {"address": 0.0.0.0", "port": 8080}})",
                             .sectionName = "network",
                             .expectSuccess = false,
                             .expectedError = Json::JsonParseError::InvalidFormat

                             },
                             LoadSectionTestCase{
                             .testName = "LoadSectionSectionNotFoundError",
                             .fileContent = R"({"network": {"address": "0.0.0.0", "port": 8080}})",
                             .sectionName = "some section",
                             .expectSuccess = false,
                             .expectedError = Json::JsonParseError::SectionNotFound
                             }), [](const ::testing::TestParamInfo<LoadSectionTestCase>& info) {
                         return info.param.testName;
                         });
