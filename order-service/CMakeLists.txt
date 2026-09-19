add_subdirectory(models)
add_subdirectory(models2json-mapper)
add_subdirectory(models2row-mappers)
add_subdirectory(repository)
add_subdirectory(handlers)

add_executable(order_service main.cpp)

find_package(CLI11 REQUIRED CONFIG)

target_link_libraries(order_service PRIVATE
    order_models
    order_models2json_mapper
    order_handlers

    shared_models
    shared_models2json_mapper
    shared_logging
    shared_http
    shared_models
    shared_outbox
    shared_messaging
    CLI11::CLI11
)
