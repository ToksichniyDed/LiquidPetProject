add_library(gateway_handlers STATIC
    ProxyHandler.cpp)

find_package(Boost REQUIRED CONFIG)

target_link_libraries(gateway_handlers PUBLIC shared_http Boost::headers)
