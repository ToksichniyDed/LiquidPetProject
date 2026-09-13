add_library(worker_events2json_mapper INTERFACE)

target_include_directories(worker_events2json_mapper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(worker_events2json_mapper INTERFACE
    keys/OrderCreatedEventJsonKeys.h

    mapper/OrderCreatedEventJsonMapper.h
    mapper/OrderReservedEventJsonMapper.h)

target_link_libraries(worker_events2json_mapper INTERFACE
worker_events
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(worker_events2json_mapper_tests
        tests/OrderCreatedEventJsonMapperTest.cpp
        tests/OrderReservedEventJsonMapperTest.cpp
    )

    target_link_libraries(worker_events2json_mapper_tests PRIVATE
        worker_events2json_mapper
        worker_events
        shared_json
        shared_models
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(worker_events2json_mapper_tests)
endif ()
