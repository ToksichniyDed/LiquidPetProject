add_library(gateway_handlers STATIC
    ProxyHandler.cpp)

find_package(Boost REQUIRED CONFIG)

target_link_libraries(gateway_handlers PUBLIC shared_http Boost::headers)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(gateway_handlers_tests
        tests/ProxyHandlerTest.cpp
    )

    target_link_libraries(gateway_handlers_tests PRIVATE
        gateway_handlers
        shared_logging
        shared_models
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(gateway_handlers_tests)
endif ()
