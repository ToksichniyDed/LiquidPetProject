//
// Created by DED on 02.10.2026.
//

#ifndef LIQUIDPETPROJECT_RECORDEXECUTOR_H
#define LIQUIDPETPROJECT_RECORDEXECUTOR_H

#include <cstdint>
#include <exception>
#include <stop_token>
#include <string>

#include <logging/Logger.h>

#include "IDeadLetterSink.h"
#include "PartitionRegistry.h"
#include "RetryPolicy.h"
#include "consumer/IEventHandler.h"

namespace shared::messaging {

struct WorkItem {
    RecordTicket ticket;
    std::string payload;
    MessageMetadata metadata;
};

enum class RecordOutcome : std::uint8_t {
    Succeeded,     // обработчик вернул true, запись завершена
    DeadLettered,  // исчерпали попытки, запись подтверждённо в DLQ, запись завершена
    Abandoned      // остановка или партиция отобрана: запись не завершена, придёт заново
};

class RecordExecutor {
   public:
    RecordExecutor(IEventHandler& handler, IDeadLetterSink& deadLetterSink, PartitionRegistry& registry,
                   RetryPolicy policy)
        : _handler(handler), _deadLetterSink(deadLetterSink), _registry(registry), _policy(policy) {}

    [[nodiscard]] RecordOutcome execute(const WorkItem& item, const std::stop_token& stopToken) {
        const int maxAttempts = _policy.effectiveMaxAttempts();

        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            if (shouldAbandon(item, stopToken))
                return RecordOutcome::Abandoned;

            if (invokeHandler(item))
                return finish(item, RecordOutcome::Succeeded);

            SPDLOG_LOGGER_WARN(shared::logger::get("RecordExecutor"), "Attempt {}/{} failed for {}@{}", attempt,
                               maxAttempts, item.ticket.partition.partition, item.ticket.offset);

            if (attempt < maxAttempts && !sleepUnlessStopped(stopToken, _policy.delayAfterAttempt(attempt)))
                return RecordOutcome::Abandoned;
        }

        return deadLetter(item, maxAttempts, stopToken);
    }

   private:
    bool shouldAbandon(const WorkItem& item, const std::stop_token& stopToken) const {
        return stopToken.stop_requested() || !_registry.isCurrent(item.ticket);
    }

    bool invokeHandler(const WorkItem& item) {
        try {
            return _handler.handle(item.payload, item.metadata);
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("RecordExecutor"), "Handler threw: {}", e.what());
        } catch (...) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("RecordExecutor"), "Handler threw unknown exception");
        }
        return false;
    }

    // DLQ недоступна - повторяем отправку, пока не получится или нас не остановят.
    RecordOutcome deadLetter(const WorkItem& item, int attempts, const std::stop_token& stopToken) {
        const DeadLetterRecord record{.source = item.ticket.partition,
                                      .offset = item.ticket.offset,
                                      .attempts = attempts,
                                      .payload = item.payload,
                                      .metadata = item.metadata};

        for (int sendAttempt = 1;; ++sendAttempt) {
            if (shouldAbandon(item, stopToken))
                return RecordOutcome::Abandoned;

            const auto sent = _deadLetterSink.send(record);
            if (sent.has_value()) {
                SPDLOG_LOGGER_ERROR(shared::logger::get("RecordExecutor"),
                                    "Record {}@{} dead-lettered after {} attempts (eventId={})",
                                    item.ticket.partition.partition, item.ticket.offset, attempts,
                                    item.metadata.eventId.value());
                return finish(item, RecordOutcome::DeadLettered);
            }

            SPDLOG_LOGGER_ERROR(shared::logger::get("RecordExecutor"), "Dead-letter send failed ({}), will retry",
                                sent.error().message());

            if (!sleepUnlessStopped(stopToken, _policy.delayAfterAttempt(sendAttempt)))
                return RecordOutcome::Abandoned;
        }
    }

    RecordOutcome finish(const WorkItem& item, RecordOutcome outcome) {
        // Stale здесь не проблема: партицию отобрали, пока обрабатывали - новый владелец разберётся сам
        if (const auto completed = _registry.complete(item.ticket); !completed.has_value()) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("RecordExecutor"), "Failed to complete {}@{}: {}",
                                item.ticket.partition.partition, item.ticket.offset, completed.error().message());
        }
        return outcome;
    }

   private:
    IEventHandler& _handler;
    IDeadLetterSink& _deadLetterSink;
    PartitionRegistry& _registry;
    RetryPolicy _policy;
};

}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_RECORDEXECUTOR_H
