find_package(libpqxx REQUIRED CONFIG)

add_library(shared_outbox STATIC
    OutboxPublisher.cpp
    PostgresOutboxRepository.cpp)

target_sources(shared_outbox PUBLIC
    IOutboxRepository.h
    OutboxRepositoryQueries.h
)

target_link_libraries(shared_outbox
    PUBLIC
    shared_models
    shared_messaging
    PRIVATE
    shared_logging
    libpqxx::pqxx
)

target_include_directories(shared_outbox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_library(shared_outbox_mocks INTERFACE)
    target_sources(shared_outbox_mocks INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        tests/MockEventPublisher.h
        tests/MockOutboxRepository.h
    )
    target_link_libraries(shared_outbox_mocks INTERFACE
        shared_outbox
        GTest::gmock
    )

    set(ORDER_REPOSITORY_TEST_SOURCES
        tests/OutboxPublisherTest.cpp
    )

    add_executable(shared_outbox_tests ${ORDER_REPOSITORY_TEST_SOURCES})
    target_link_libraries(shared_outbox_tests PRIVATE
        shared_outbox
        shared_outbox_mocks
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_outbox_tests)
endif ()
