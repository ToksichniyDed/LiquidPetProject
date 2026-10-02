//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_OUTBOXPUBLISHER_H
#define LIQUIDPETPROJECT_OUTBOXPUBLISHER_H

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <producer/IEventPublisher.h>
#include "IOutboxRepository.h"

namespace shared::outbox {

class OutboxPublisher {
public:
    OutboxPublisher(
        std::shared_ptr<IOutboxRepository> repository,
        std::shared_ptr<shared::messaging::IEventPublisher> publisher,
        std::string topic,
        std::chrono::milliseconds pollInterval = std::chrono::milliseconds(500),
        int batchSize = 100,
        std::chrono::milliseconds publishTimeout = std::chrono::milliseconds(5000));

    void start();
    void stop();

public:
    struct BatchResult {
        std::size_t fetched = 0; // сколько записей достали из outbox
        std::size_t published = 0; // сколько из них реально ушло в Kafka и помечено

        // Есть смысл брать следующую пачку сразу: прошлая была полной
        // и хоть что-то ушло
        [[nodiscard]] bool hasMoreWork(const int batchSize) const {
            return published > 0 && std::cmp_greater_equal(fetched, batchSize);
        }
    };

private:
    struct PendingPublish {
        OutboxEntry entry;
        std::future<std::expected<void, std::error_code>> result;
    };

    void run(const std::stop_token& stopToken) const;
    [[nodiscard]] BatchResult  processBatch() const;

private:
    std::shared_ptr<IOutboxRepository> _repository;
    std::shared_ptr<messaging::IEventPublisher> _publisher;
    std::string _topic;
    std::chrono::milliseconds _pollInterval;
    std::chrono::milliseconds _publishTimeout;
    int _batchSize;

    std::jthread _thread;
};
}


#endif //LIQUIDPETPROJECT_OUTBOXPUBLISHER_H
