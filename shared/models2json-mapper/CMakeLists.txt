add_library(shared_models2json_mapper INTERFACE
)

target_sources(shared_models2json_mapper INTERFACE
    keys/DatabaseConfigurationJsonKeys.h
    mapper/DatabaseConfigurationJsonMapper.h
)

target_include_directories(shared_models2json_mapper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shared_models2json_mapper INTERFACE
    shared_models
    shared_json
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(shared_models2json_mapper_tests
        tests/DatabaseConfigurationJsonMapperTest.cpp
    )

    target_link_libraries(shared_models2json_mapper_tests PUBLIC
        shared_models
        PRIVATE
        shared_models2json_mapper
        shared_json
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_models2json_mapper_tests)
endif ()
