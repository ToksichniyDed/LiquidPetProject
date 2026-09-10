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

target_link_libraries(worker_repository PRIVATE
    worker_events2json_mapper
    worker_events
    worker_models

    shared_logging
    shared_models
    shared_include_paths
    libpqxx::pqxx
)
