//
// Created by DED on 02.10.2026.
//

#ifndef LIQUIDPETPROJECT_CONSUMERPIPELINE_H
#define LIQUIDPETPROJECT_CONSUMERPIPELINE_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <logging/Logger.h>
#include <models/OrderIds.h>

#include "PartitionRegistry.h"
#include "RecordExecutor.h"
#include "WorkerPool.h"

namespace shared::messaging {

// Запись в том виде, в котором её отдаёт адаптер брокера
struct IncomingRecord {
    PartitionKey partition;
    std::int64_t offset;
    std::string payload;
    std::optional<std::string> eventId;  // сырое значение заголовка, валидируется здесь
    std::string eventType;
};

struct PipelineConfiguration {
    std::size_t workerThreads = 4;
    std::size_t queueCapacity = 64;
    RetryPolicy retry{};
};

class ConsumerPipeline {
   public:
    ConsumerPipeline(IEventHandler& handler, IDeadLetterSink& deadLetterSink, const PipelineConfiguration& config)
        : _executor(handler, deadLetterSink, _registry, config.retry),
          _pool(config.workerThreads, config.queueCapacity) {}

    ~ConsumerPipeline() { stop(); }

    ConsumerPipeline(const ConsumerPipeline&) = delete;
    ConsumerPipeline& operator=(const ConsumerPipeline&) = delete;

    void onPartitionsAssigned(const std::vector<PartitionKey>& partitions) { _registry.assign(partitions); }

    // Возвращает финальные позиции для коммита в rebalance-колбэке
    [[nodiscard]] std::vector<CommitPosition> onPartitionsRevoked(const std::vector<PartitionKey>& partitions) {
        auto finalPositions = _registry.revoke(partitions);

        // Записи отобранных партиций, не ушедшие в пул, больше не нужны: их билеты уже Stale
        std::erase_if(_backlog, [&partitions](const auto& item) {
            return std::ranges::contains(partitions, item->ticket.partition);
        });
        return finalPositions;
    }

    void ingest(IncomingRecord record) {
        auto ticket = _registry.registerRecord(record.partition, record.offset);
        if (!ticket.has_value()) {
            SPDLOG_LOGGER_WARN(shared::logger::get("ConsumerPipeline"), "Record {}@{} not registered: {}",
                               record.partition.partition, record.offset, ticket.error().message());
            return;
        }

        auto eventId = models::OutboxEventId::create(record.eventId.value_or(""));
        if (!eventId.has_value()) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("ConsumerPipeline"),
                                "Record {}@{} has no valid eventId, skipping", record.partition.partition,
                                record.offset);
            std::ignore = _registry.complete(*ticket);
            return;
        }

        _backlog.push_back(std::make_shared<const WorkItem>(
            WorkItem{.ticket = *ticket,
                     .payload = std::move(record.payload),
                     .metadata = MessageMetadata{.eventId = std::move(*eventId),
                                                 .eventType = std::move(record.eventType)}}));
    }

    // Отправляет backlog в пул. true - backlog пуст, можно читать дальше; false - пул полон, читателю пора на паузу
    [[nodiscard]] bool pump() {
        while (!_backlog.empty()) {
            const auto item = _backlog.front();  // копия shared_ptr: при отказе пула запись остаётся в backlog
            const bool accepted = _pool.trySubmit([this, item](const std::stop_token& stopToken) {
                std::ignore = _executor.execute(*item, stopToken);
            });
            if (!accepted)
                return false;
            _backlog.pop_front();
        }
        return true;
    }

    [[nodiscard]] std::vector<CommitPosition> commitCandidates() const { return _registry.commitCandidates(); }

    void confirmCommit(const CommitPosition& position) {
        if (const auto confirmed = _registry.confirmCommit(position); !confirmed.has_value()) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("ConsumerPipeline"), "Commit confirmation failed: {}",
                                confirmed.error().message());
        }
    }

    [[nodiscard]] std::size_t backlogSize() const { return _backlog.size(); }

    // Выполняющиеся записи получают stop_token, ещё не начатые отбрасываются (придут заново)
    void stop() {
        _pool.stop();
        _backlog.clear();
    }

   private:
    PartitionRegistry _registry;
    RecordExecutor _executor;
    std::deque<std::shared_ptr<const WorkItem>> _backlog;
    WorkerPool _pool;
};

}

#endif  // LIQUIDPETPROJECT_CONSUMERPIPELINE_H
