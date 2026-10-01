//
// Created by DED on 01.10.2026.
//

#ifndef LIQUIDPETPROJECT_PARTITIONREGISTRY_H
#define LIQUIDPETPROJECT_PARTITIONREGISTRY_H

#include <cstdint>
#include <expected>
#include <map>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "OffsetTracker.h"

namespace shared::messaging {
enum class PartitionRegistryError : std::uint8_t { PartitionNotAssigned = 1 };

class PartitionRegistryErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override { return "partition_registry"; }

    std::string message(int ev) const override {
        switch (static_cast<PartitionRegistryError>(ev)) {
            case PartitionRegistryError::PartitionNotAssigned:
                return "partition is not assigned";
            default:
                return "unknown partition registry error";
        }
    }
};

inline const PartitionRegistryErrorCategory& partitionRegistryErrorCategory() {
    static PartitionRegistryErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(PartitionRegistryError e) {
    return {std::to_underlying(e), partitionRegistryErrorCategory()};
}
}

namespace std {
template <>
struct is_error_code_enum<shared::messaging::PartitionRegistryError> : true_type {
};
}

namespace shared::messaging {
struct PartitionKey {
    std::string topic;
    std::int32_t partition;

    auto operator<=>(const PartitionKey&) const = default;
};

struct RecordTicket {
    PartitionKey partition;
    std::int64_t offset;
    std::uint64_t generation;
};

struct CommitPosition {
    PartitionKey partition;
    std::int64_t offset;
    std::uint64_t generation;
};

enum class CompletionResult : std::uint8_t {
    Applied, // завершение учтено
    Stale // билет от прошлого владения партицией - проигнорирован
};

// Реестр трекеров всех назначенных партиций.
// assign/revoke/registerRecord/commitCandidates/confirmCommit вызываются из потока-читателя,
// complete - из потоков-воркеров, поэтому всё под одним мьютексом.
class PartitionRegistry {
public:
    // Партиция назначена. Всегда начинает с чистого листа и нового поколения:
    // чтение продолжится с закоммиченной позиции, старое состояние недействительно.
    void assign(const std::vector<PartitionKey>& partitions) {
        std::scoped_lock lock(_mutex);
        for (const auto& partition : partitions) {
            _partitions.insert_or_assign(partition,
                                         PartitionState{.generation = ++_lastGeneration, .tracker = OffsetTracker{}});
        }
    }

    // Партиция отобрана. Возвращает последние позиции, которые ещё можно закоммитить
    // Незавершённые записи отбрасываются
    [[nodiscard]] std::vector<CommitPosition> revoke(const std::vector<PartitionKey>& partitions) {
        std::scoped_lock lock(_mutex);

        std::vector<CommitPosition> finalPositions;
        for (const auto& partition : partitions) {
            const auto it = _partitions.find(partition);
            if (it == _partitions.end())
                continue;

            if (const auto candidate = it->second.tracker.commitCandidate(); candidate.has_value()) {
                finalPositions.push_back(CommitPosition{.partition = partition,
                                                        .offset = *candidate,
                                                        .generation = it->second.generation});
            }
            _partitions.erase(it);
        }
        return finalPositions;
    }

    [[nodiscard]] std::expected<RecordTicket, std::error_code> registerRecord(const PartitionKey& partition,
                                                                              std::int64_t offset) {
        std::scoped_lock lock(_mutex);

        const auto it = _partitions.find(partition);
        if (it == _partitions.end())
            return std::unexpected(PartitionRegistryError::PartitionNotAssigned);

        const auto generation = it->second.generation;
        return it->second.tracker.registerRecord(offset).transform([&] {
            return RecordTicket{.partition = partition, .offset = offset, .generation = generation};
        });
    }

    [[nodiscard]] std::expected<CompletionResult, std::error_code> complete(const RecordTicket& ticket) {
        std::scoped_lock lock(_mutex);

        const auto it = _partitions.find(ticket.partition);
        if (it == _partitions.end() || it->second.generation != ticket.generation)
            return CompletionResult::Stale;

        return it->second.tracker.complete(ticket.offset).transform([] { return CompletionResult::Applied; });
    }

    // Что можно коммитить прямо сейчас
    [[nodiscard]] std::vector<CommitPosition> commitCandidates() const {
        std::scoped_lock lock(_mutex);

        std::vector<CommitPosition> positions;
        for (const auto& [partition, state] : _partitions) {
            if (const auto candidate = state.tracker.commitCandidate(); candidate.has_value()) {
                positions.push_back(CommitPosition{.partition = partition,
                                                   .offset = *candidate,
                                                   .generation = state.generation});
            }
        }
        return positions;
    }

    // Брокер принял коммит. Если партицию за это время отобрали или переназначили - игнорируем.
    [[nodiscard]] std::expected<void, std::error_code> confirmCommit(const CommitPosition& position) {
        std::scoped_lock lock(_mutex);

        const auto it = _partitions.find(position.partition);
        if (it == _partitions.end() || it->second.generation != position.generation)
            return {};

        return it->second.tracker.confirmCommit(position.offset);
    }

    [[nodiscard]] bool isAssigned(const PartitionKey& partition) const {
        std::scoped_lock lock(_mutex);
        return _partitions.contains(partition);
    }

private:
    struct PartitionState {
        std::uint64_t generation;
        OffsetTracker tracker;
    };

private:
    mutable std::mutex _mutex;
    std::map<PartitionKey, PartitionState> _partitions;
    std::uint64_t _lastGeneration = 0;
};
}

#endif  // LIQUIDPETPROJECT_PARTITIONREGISTRY_H
