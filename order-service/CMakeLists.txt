add_subdirectory(models)
add_subdirectory(models2json-mapper)
add_subdirectory(repository)
# add_subdirectory(db)
add_subdirectory(handlers)

add_executable(order_service main.cpp)

target_include_directories(order_service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_service PRIVATE
    order_models
    order_repository
    order_handlers
    order-models2json-mapper
    shared
)
