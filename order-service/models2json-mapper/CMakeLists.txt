add_library(order_models2json_mapper INTERFACE
)

target_include_directories(order_models2json_mapper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_models2json_mapper INTERFACE
    order_models
    shared_json
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(order_models2json_mapper_tests
        tests/MoneyJsonMapperTest.cpp
        tests/CurrencyJsonMapperTest.cpp
        tests/OrderJsonMapperTest.cpp
        tests/NetworkConfigurationJsonMapperTest.cpp
        tests/DatabaseConfigurationJsonMapperTest.cpp
    )

    target_link_libraries(order_models2json_mapper_tests PUBLIC
        order_models
        PRIVATE
        order_models2json_mapper
        shared_json
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_models2json_mapper_tests)
endif ()
