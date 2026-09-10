add_library(worker_events INTERFACE)

target_include_directories(worker_events INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(worker_events INTERFACE
    OrderCreatedEvent.h
    OrderReservedEvent.h
)

target_link_libraries(worker_events INTERFACE
    shared_json
    shared_models
)
