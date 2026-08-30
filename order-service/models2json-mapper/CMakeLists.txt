add_library(order_models2json_mapper STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mapper
    ${CMAKE_CURRENT_SOURCE_DIR}/keys
)

target_include_directories(order_models2json_mapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_models2json_mapper PUBLIC
    order_models
    PRIVATE
    shared
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(order_models2json_mapper_tests
        tests/MoneyJsonMapperTest.cpp
        tests/CurrencyJsonMapperTest.cpp
        tests/OrderJsonMapperTest.cpp
    )

    target_link_libraries(order_models2json_mapper_tests PUBLIC
        order_models
        PRIVATE
        order_models2json_mapper
        shared
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_models2json_mapper_tests)
endif ()
