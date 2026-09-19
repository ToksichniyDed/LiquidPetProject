find_package(modern-cpp-kafka REQUIRED CONFIG)

add_library(shared_messaging STATIC
    consumer/KafkaEventConsumer.cpp
    producer/KafkaEventPublisher.cpp)

target_include_directories(shared_messaging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(shared_messaging PUBLIC
    PublishRequest.h
    MessageMetadata.h
    consumer/IEventConsumer.h
    consumer/IEventHandler.h
    producer/IEventPublisher.h
)

target_link_libraries(shared_messaging PRIVATE shared_logging shared_models modern-cpp-kafka::modern-cpp-kafka)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(shared_messaging_integration_tests
            tests/KafkaEventConsumerIntegrationTest.cpp
    )

    target_link_libraries(shared_messaging_integration_tests PRIVATE
            shared_messaging
            shared_logging
            shared_models
            shared_include_paths
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_messaging_integration_tests
            PROPERTIES LABELS integration
            RUN_SERIAL TRUE
    )
endif ()
