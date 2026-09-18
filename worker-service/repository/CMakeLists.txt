find_package(libpqxx REQUIRED CONFIG)

add_library(worker_repository STATIC
        PostgresWorkerRepository.cpp)

target_include_directories(worker_repository INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(worker_repository PUBLIC
        IWorkerRepository.h
        PostgresWorkerRepository.h
        WorkerRepositoryQueries.h)

target_link_libraries(worker_repository PUBLIC
        shared_repository
        shared_models

        PRIVATE
        worker_events2json_mapper
        worker_events

        shared_logging
        shared_repository_postgres
        shared_include_paths
        libpqxx::pqxx
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(worker_repository_integration_tests
            tests/PostgresWorkerRepositoryIntegrationTest.cpp
    )
    target_link_libraries(worker_repository_integration_tests PRIVATE
            worker_repository
            shared_include_paths
            shared_http
            libpqxx::pqxx
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(worker_repository_integration_tests
            PROPERTIES LABELS integration
            RUN_SERIAL TRUE
    )
endif ()
