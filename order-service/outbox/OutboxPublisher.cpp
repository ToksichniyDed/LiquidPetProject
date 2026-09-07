//
// Created by DED on 06.09.2026.
//

#include "OutboxPublisher.h"

#include <logging/Logger.h>

namespace order_service::outbox {
    OutboxPublisher::OutboxPublisher(
        std::shared_ptr<IOutboxRepository> repository,
        std::shared_ptr<messaging::IEventPublisher> publisher,
        std::string topic,
        std::chrono::milliseconds pollInterval,
        const int batchSize, std::chrono::milliseconds publishTimeout) : _repository(std::move(repository)),
                                                                         _publisher(std::move(publisher)),
                                                                         _topic(std::move(topic)),
                                                                         _pollInterval(pollInterval),
                                                                         _publishTimeout(publishTimeout),
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

        if (!entries.has_value()) {
            SPDLOG_LOGGER_ERROR(Logger::get("OutboxPublisher"),
                                "Fetch unpublished entries failed: {}", entries.error().message());
            return;
        }

        if (entries.value().empty()) {
            return;
        }

        std::vector<PendingPublish> pending;
        pending.reserve(entries.value().size());

        for (const auto& entry : entries.value()) {
            pending.push_back(PendingPublish{
                .entry = entry,
                .result = _publisher->publish(_topic, entry.aggregateId, entry.payload)
            });
        }

        for (auto& [entry, result] : pending) {

            if (const auto waitStatus = result.wait_for(_publishTimeout); waitStatus != std::future_status::ready) {
                SPDLOG_LOGGER_WARN(Logger::get("OutboxPublisher"),
                                   "Publish for outbox entry {} did not complete within {} ms, will retry next cycle",
                                   entry.id, _publishTimeout.count());
                continue;
            }


            if (const auto publishResult = result.get(); !publishResult.has_value()) {
                SPDLOG_LOGGER_WARN(Logger::get("OutboxPublisher"),
                                   "Failed to publish outbox entry {}: {}, will retry next cycle",
                                   entry.id, publishResult.error().message());
                continue;
            }

            _repository->markAsPublished(entry.id).or_else([&entry](const std::error_code& ec) {
                SPDLOG_LOGGER_WARN(Logger::get("OutboxPublisher"),
                                   "Failed to mark entry {} as published: {}", entry.id, ec.message());
                return std::expected<void, std::error_code>{std::unexpected(ec)};
            });
        }
    }
}
