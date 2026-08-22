add_library(order_models INTERFACE)

target_include_directories(order_models INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(order_models INTERFACE cxx_std_23)

find_package(Boost REQUIRED CONFIG)
target_link_libraries(order_models INTERFACE
    Boost::headers
)

if(BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(order_models_tests
        tests/MoneyTest.cpp
        tests/CurrencyTest.cpp
    )

    target_link_libraries(order_models_tests PRIVATE
        order_models
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_models_tests)
endif()
