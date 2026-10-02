find_package(modern-cpp-kafka REQUIRED CONFIG)

add_library(shared_messaging STATIC
        consumer/KafkaEventConsumer.cpp
        producer/KafkaEventPublisher.cpp)

target_include_directories(shared_messaging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(shared_messaging PUBLIC
        PublishRequest.h
        MessageMetadata.h
        consumer/IEventConsumer.h
        consumer/IEventHandler.h
        producer/IEventPublisher.h
)

target_link_libraries(shared_messaging PRIVATE shared_logging shared_models modern-cpp-kafka::modern-cpp-kafka)

add_library(shared_messaging_processing INTERFACE)
target_include_directories(shared_messaging_processing INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(shared_messaging_processing INTERFACE cxx_std_23)
target_sources(shared_messaging_processing INTERFACE
        OffsetTracker.h
        PartitionRegistry.h
        RetryPolicy.h
        WorkerPool.h
        IDeadLetterSink.h
        PublisherDeadLetterSink.h
        RecordExecutor.h
        ConsumerPipeline.h
)
target_link_libraries(shared_messaging_processing INTERFACE
        shared_include_paths
        shared_models
        shared_logging
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)
    include(GoogleTest)

    add_executable(shared_messaging_tests
            tests/OffsetTrackerTest.cpp
            tests/PartitionRegistryTest.cpp
            tests/RetryPolicyTest.cpp
            tests/WorkerPoolTest.cpp
            tests/RecordExecutorTest.cpp
            tests/ConsumerPipelineTest.cpp
    )
    target_link_libraries(shared_messaging_tests PRIVATE
            shared_messaging_processing
            GTest::gmock
            GTest::gtest_main
    )
    gtest_discover_tests(shared_messaging_tests)

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
    gtest_discover_tests(shared_messaging_integration_tests
            PROPERTIES LABELS integration
            RUN_SERIAL TRUE
    )
endif ()
