//
// Created by DED on 13.09.2026.
//

#include <gtest/gtest.h>
#include <postgres/PqxxExceptionMapper.h>

using namespace shared::repository;
using namespace shared::repository::postgres;

namespace {
struct MapPqxxExceptionTestCase {
    std::string testName;
    std::exception_ptr exception;
    RepositoryError expectedError;
};
}  // namespace

class MapPqxxExceptionTest : public ::testing::TestWithParam<MapPqxxExceptionTestCase> {};

TEST_P(MapPqxxExceptionTest, MapsToExpectedRepositoryError) {
    const auto& testCase = GetParam();

    try {
        std::rethrow_exception(testCase.exception);
    } catch (const std::exception& e) {
        const auto errorCode = mapPqxxException(e);
        EXPECT_EQ(errorCode, testCase.expectedError);
    }
}

INSTANTIATE_TEST_SUITE_P(
    PqxxExceptionMapperTests, MapPqxxExceptionTest,
    ::testing::Values(MapPqxxExceptionTestCase{.testName = "BrokenConnectionMapsToConnectionFailure",
                                               .exception = std::make_exception_ptr(pqxx::broken_connection{}),
                                               .expectedError = RepositoryError::ConnectionFailure},
                      MapPqxxExceptionTestCase{.testName = "UniqueViolationMapsToConstraintViolation",
                                               .exception = std::make_exception_ptr(pqxx::unique_violation{{}}),
                                               .expectedError = RepositoryError::ConstraintViolation},
                      MapPqxxExceptionTestCase{.testName = "ForeignKeyViolationMapsToConstraintViolation",
                                               .exception = std::make_exception_ptr(pqxx::foreign_key_violation{{}}),
                                               .expectedError = RepositoryError::ConstraintViolation},
                      MapPqxxExceptionTestCase{.testName = "CheckViolationMapsToConstraintViolation",
                                               .exception = std::make_exception_ptr(pqxx::check_violation{{}}),
                                               .expectedError = RepositoryError::ConstraintViolation},
                      MapPqxxExceptionTestCase{.testName = "InDoubtErrorMapsToTimeout",
                                               .exception = std::make_exception_ptr(pqxx::in_doubt_error{{}}),
                                               .expectedError = RepositoryError::Timeout},
                      MapPqxxExceptionTestCase{.testName = "UnknownExceptionMapsToSerializationFailure",
                                               .exception = std::make_exception_ptr(std::runtime_error{"unexpected"}),
                                               .expectedError = RepositoryError::SerializationFailure}),
    [](const ::testing::TestParamInfo<MapPqxxExceptionTestCase>& info) { return info.param.testName; });
