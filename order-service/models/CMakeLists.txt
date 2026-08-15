add_library(order_models INTERFACE)

target_include_directories(order_models INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(order_models INTERFACE cxx_std_23)