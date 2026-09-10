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
