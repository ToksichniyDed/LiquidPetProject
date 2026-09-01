set(ORDER_HANDLERS_HEADERS
    Route.h
    RoutePaths.h
    HttpServer.h
    IRequestHandler.h
    HttpMessageConverter.h
    CQRS/HealthHandler.h
    CQRS/GetOrderHandler.h
    CQRS/CreateOrderHandler.h
)

set(ORDER_HANDLERS_SOURCES
    HttpServer.cpp
    HttpMessageConverter.cpp)

add_library(order_handlers STATIC
    ${ORDER_HANDLERS_SOURCES}
)

target_include_directories(order_handlers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Boost REQUIRED CONFIG)

target_link_libraries(order_handlers
    PUBLIC
    order_models
    order_models2json_mapper
    order_repository
    shared
    PRIVATE
    Boost::headers
)

