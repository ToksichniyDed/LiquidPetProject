add_library(order_models2json_mapper STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mapper
    ${CMAKE_CURRENT_SOURCE_DIR}/keys
)

target_include_directories(order_models2json_mapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order_models2json_mapper PRIVATE
    shared
    order_models
)
