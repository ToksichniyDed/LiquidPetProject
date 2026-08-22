set(ORDER_HANDLERS_HEADERS
HttpServer.h)

set(ORDER_HANDLERS_SOURCES
HttpServer.cpp)

add_library(order_handlers STATIC ${ORDER_HANDLERS_SOURCES})

find_package(Boost REQUIRED CONFIG)

target_link_libraries(order_handlers PRIVATE
    order_models
    Boost::headers
)

