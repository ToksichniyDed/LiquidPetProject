add_subdirectory(models)
add_subdirectory(models2json-mapper)
add_subdirectory(models2row-mappers)
add_subdirectory(repository)
add_subdirectory(handlers)

add_executable(order_service main.cpp)

target_link_libraries(order_service PRIVATE
    order_models
    order_repository
    order_handlers
    order_models2json_mapper
    shared_logging
    shared_http
)
