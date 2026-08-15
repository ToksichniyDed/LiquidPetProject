add_subdirectory(models)
# add_subdirectory(db)
# add_subdirectory(repository)
# add_subdirectory(handlers)

add_executable(order_service main.cpp)

target_link_libraries(order_service PRIVATE
    order_models
    shared
)