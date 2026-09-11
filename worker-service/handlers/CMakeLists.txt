add_library(worker_handlers STATIC
    OrderReservationHandler.cpp)

target_link_libraries(worker_handlers PUBLIC
    worker_repository
    worker_processing
    worker_events2json_mapper

    shared_messaging
    shared_logging)
