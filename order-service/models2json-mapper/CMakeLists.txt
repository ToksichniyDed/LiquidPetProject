add_library(order-models2json-mapper STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mapper
    ${CMAKE_CURRENT_SOURCE_DIR}/keys
)

target_include_directories(order-models2json-mapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(order-models2json-mapper PRIVATE
    shared
    order_models
)
