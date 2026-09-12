add_subdirectory(events)
add_subdirectory(events2json-mapper)
add_subdirectory(repository)
add_subdirectory(handlers)
add_subdirectory(processing)

add_executable(worker_service main.cpp)

target_link_libraries(worker_service PRIVATE
    worker_handlers
    worker_processing
    worker_repository

    shared_logging
    shared_messaging
    shared_outbox
    shared_models
    shared_json
)
