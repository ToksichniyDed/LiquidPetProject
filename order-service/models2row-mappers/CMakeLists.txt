find_package(libpqxx REQUIRED CONFIG)

add_library(order_models2row_mapper STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mapper
    ${CMAKE_CURRENT_SOURCE_DIR}/columns
)

target_include_directories(order_models2row_mapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_models2row_mapper
    PUBLIC
    order_models
    PRIVATE
    libpqxx::pqxx
)
