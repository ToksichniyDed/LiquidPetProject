find_package(libpqxx REQUIRED CONFIG)

add_library(order_outbox STATIC
    OutboxPublisher.cpp
    PostgresOutboxRepository.cpp)

target_sources(order_outbox PUBLIC
    IEventPublisher.h
    IOutboxRepository.h
    OutboxRepositoryQueries.h
)

target_link_libraries(order_outbox PRIVATE shared_logging order_models libpqxx::pqxx)
target_include_directories(order_outbox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_library(order_outbox_mocks INTERFACE)
    target_sources(order_outbox_mocks INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        tests/MockEventPublisher.h
        tests/MockOutboxRepository.h
    )
    target_link_libraries(order_outbox_mocks INTERFACE
        order_outbox
        GTest::gmock
    )

    set(ORDER_REPOSITORY_TEST_SOURCES
        tests/OutboxPublisherTest.cpp
    )

    add_executable(order_outbox_tests ${ORDER_REPOSITORY_TEST_SOURCES})
    target_link_libraries(order_outbox_tests PRIVATE
        order_outbox
        order_outbox_mocks
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_outbox_tests)
endif ()
