//
// Created by DED on 06.09.2026.
//

#include "OutboxPublisher.h"

#include <logging/Logger.h>

namespace order_service::outbox {
    OutboxPublisher::OutboxPublisher(
        std::shared_ptr<IOutboxRepository> repository,
        std::shared_ptr<IEventPublisher> publisher,
        std::string topic,
        const std::chrono::milliseconds pollInterval,
        const int batchSize) : _repository(std::move(repository)),
                               _publisher(std::move(publisher)),
                               _topic(std::move(topic)),
                               _pollInterval(pollInterval),
                               _batchSize(batchSize) {
    }

    void OutboxPublisher::start() {
        SPDLOG_LOGGER_INFO(Logger::get("OutboxPublisher"), "Starting outbox publisher");
        _thread = std::jthread([this](std::stop_token stopToken) {
            run(stopToken);
        });
    }

    void OutboxPublisher::stop() {
        SPDLOG_LOGGER_INFO(Logger::get("OutboxPublisher"), "Stop outbox publisher");
        _thread.request_stop();
    }

    void OutboxPublisher::run(const std::stop_token& stopToken) const {
        while (!stopToken.stop_requested()) {
            processBatch();
            std::this_thread::sleep_for(_pollInterval);
        }
        SPDLOG_LOGGER_INFO(Logger::get("OutboxPublisher"), "Publisher loop finished");
    }

    void OutboxPublisher::processBatch() const {
        const auto entries = _repository->fetchUnpublished(_batchSize);

        for (const auto& entry : entries) {
            const bool success = _publisher->publish(_topic, entry.aggregateId, entry.payload);

            if (success)
                _repository->markAsPublished(entry.id);
            else
                SPDLOG_LOGGER_WARN(Logger::get("OutboxPublisher"),
                               "Failed to publish outbox entry {}, will retry next cycle", entry.id);
        }
    }

}
