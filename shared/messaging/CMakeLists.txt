find_package(modern-cpp-kafka REQUIRED CONFIG)

add_library(shared_messaging STATIC
consumer/KafkaEventConsumer.cpp
producer/KafkaEventPublisher.cpp)

target_sources(shared_messaging PUBLIC
consumer/IEventConsumer.h
consumer/IEventHandler.h
producer/IEventPublisher.h
)

target_include_directories(shared_messaging INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shared_messaging PRIVATE shared_logging shared_models modern-cpp-kafka::modern-cpp-kafka)
