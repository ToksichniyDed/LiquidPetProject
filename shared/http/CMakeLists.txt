add_library(shared_http STATIC
    HttpMessageConverter.cpp
    HttpServer.cpp)

find_package(Boost REQUIRED CONFIG)

target_link_libraries(shared_http PUBLIC shared_include_paths PRIVATE shared_logging Boost::headers)
