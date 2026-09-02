find_package(libpqxx REQUIRED CONFIG)

add_library(order_models2row_mapper INTERFACE
)

target_include_directories(order_models2row_mapper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_models2row_mapper
    INTERFACE
    order_models
    libpqxx::pqxx
)
