add_library(order_outbox STATIC
    OutboxPublisher.cpp)

target_sources(order_outbox PUBLIC
    IEventPublisher.h
    IOutboxRepository.h
)

target_link_libraries(order_outbox PRIVATE shared_logging)
