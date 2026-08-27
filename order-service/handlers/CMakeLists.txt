set(ORDER_HANDLERS_HEADERS
    HttpServer.h
    IRequestHandler.h
    HttpMessageConverter.h
)

set(ORDER_HANDLERS_SOURCES
    HttpServer.cpp
    HttpMessageConverter.cpp)

add_library(order_handlers STATIC ${ORDER_HANDLERS_SOURCES})

find_package(Boost REQUIRED CONFIG)

target_link_libraries(order_handlers PRIVATE
    order_models
    shared
    Boost::headers
)

