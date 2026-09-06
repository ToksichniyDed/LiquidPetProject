//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_OUTBOXPUBLISHER_H
#define LIQUIDPETPROJECT_OUTBOXPUBLISHER_H

#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "IEventPublisher.h"
#include "IOutboxRepository.h"

namespace order_service::outbox {

    class OutboxPublisher {
    public:
        OutboxPublisher(
            std::shared_ptr<IOutboxRepository> repository,
            std::shared_ptr<IEventPublisher> publisher,
            std::string topic,
            std::chrono::milliseconds pollInterval = std::chrono::milliseconds(500),
            int batchSize = 100);

        void start();
        void stop();

    private:
        void run(const std::stop_token& stopToken) const;
        void processBatch() const;

    private:
        std::shared_ptr<IOutboxRepository> _repository;
        std::shared_ptr<IEventPublisher> _publisher;
        std::string _topic;
        std::chrono::milliseconds _pollInterval;
        int _batchSize;

        std::jthread _thread;
    };

}


#endif //LIQUIDPETPROJECT_OUTBOXPUBLISHER_H
