add_library(order_event_handlers STATIC
        OrderReservedHandler.cpp
)

target_include_directories(order_event_handlers PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_event_handlers PUBLIC
        order_models
        order_repository
        shared_messaging
        shared_json
        shared_logging
        shared_include_paths
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(order_event_handlers_tests
            tests/OrderReservedEventJsonMapperTest.cpp
            tests/OrderReservedHandlerTest.cpp
    )

    target_link_libraries(order_event_handlers_tests PRIVATE
            order_event_handlers
            order_repository_mocks
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_event_handlers_tests)
endif ()
