find_package(libpqxx CONFIG REQUIRED)

add_library(shared_repository_postgres INTERFACE)

target_include_directories(shared_repository_postgres INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(shared_repository_postgres INTERFACE
    PqxxExceptionMapper.h)

target_link_libraries(shared_repository_postgres INTERFACE shared_include_paths shared_repository libpqxx::pqxx)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(shared_repository_postgres_tests
        tests/PqxxExceptionMapperTest.cpp
    )

    target_link_libraries(shared_repository_postgres_tests PRIVATE
        shared_repository_postgres
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_repository_postgres_tests)
endif ()
