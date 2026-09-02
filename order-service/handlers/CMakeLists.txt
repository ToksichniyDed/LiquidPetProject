add_library(order_handlers INTERFACE)

find_package(Boost REQUIRED CONFIG)

target_include_directories(order_handlers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_handlers
    INTERFACE
    order_repository
    order_models
    order_models2json_mapper
    Boost::headers
)

