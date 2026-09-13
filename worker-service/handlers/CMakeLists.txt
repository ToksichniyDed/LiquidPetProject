add_library(worker_handlers STATIC
    OrderReservationHandler.cpp)

target_link_libraries(worker_handlers PUBLIC
    worker_repository
    worker_processing
    worker_events2json_mapper

    shared_messaging
    shared_logging)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_library(worker_handlers_mocks INTERFACE)
    target_sources(worker_handlers_mocks INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        tests/MockWorkerRepository.h
        tests/MockOrderProcessor.h
    )
    target_link_libraries(worker_handlers_mocks INTERFACE
        worker_handlers
        GTest::gmock
    )

    add_executable(worker_handlers_tests
        tests/OrderReservationHandlerTest.cpp
    )

    target_link_libraries(worker_handlers_tests PRIVATE
        worker_handlers_mocks
        worker_events2json_mapper
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(worker_handlers_tests)
endif ()
