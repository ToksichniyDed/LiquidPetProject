find_package(modern-cpp-kafka REQUIRED CONFIG)

add_library(shared_messaging STATIC
KafkaEventPublisher.cpp)

target_sources(shared_messaging PUBLIC
IEventPublisher.h)

target_link_libraries(shared_messaging PRIVATE shared_logging modern-cpp-kafka::modern-cpp-kafka)
