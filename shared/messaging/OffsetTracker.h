//
// Created by DED on 30.09.2026.
//

#ifndef LIQUIDPETPROJECT_OFFSETTRACKER_H
#define LIQUIDPETPROJECT_OFFSETTRACKER_H

#include <algorithm>
#include <cstdint>
#include <deque>
#include <expected>
#include <optional>
#include <string>
#include <system_error>

namespace shared::messaging {
enum class OffsetTrackerError : std::uint8_t {
    OutOfOrderRegistration = 1,
    UnknownOffset,
    CommitBeyondWatermark
};

class OffsetTrackerErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override { return "offset_tracker"; }

    std::string message(int ev) const override {
        switch (static_cast<OffsetTrackerError>(ev)) {
            case OffsetTrackerError::OutOfOrderRegistration:
                return "offset registered out of order";
            case OffsetTrackerError::UnknownOffset:
                return "unknown offset";
            case OffsetTrackerError::CommitBeyondWatermark:
                return "commit position is beyond watermark";
            default:
                return "unknown offset tracker error";
        }
    }
};

inline const OffsetTrackerErrorCategory& offsetTrackerErrorCategory() {
    static OffsetTrackerErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(OffsetTrackerError e) {
    return {static_cast<int>(e), offsetTrackerErrorCategory()};
}
}

namespace std {
template <>
struct is_error_code_enum<shared::messaging::OffsetTrackerError> : true_type {
};
}

namespace shared::messaging {
// Трекер оффсетов одной партиции. Не потокобезопасен: все вызовы из потока-читателя.
class OffsetTracker {
public:
    // Запись ушла в обработку. Оффсеты обязаны строго возрастать.
    [[nodiscard]] std::expected<void, std::error_code> registerRecord(std::int64_t offset) {
        if (_lastRegistered.has_value() && offset <= *_lastRegistered)
            return std::unexpected(OffsetTrackerError::OutOfOrderRegistration);

        _entries.push_back(Entry{.offset = offset, .done = false});
        _lastRegistered = offset;
        ++_inFlight;
        return {};
    }

    // Обработка записи закончена (успех или окончательный отказ).
    [[nodiscard]] std::expected<void, std::error_code> complete(std::int64_t offset) {
        // Запись уже покрыта коммитом и удалена из очереди - повторное завершение игнорируем
        if (_committed.has_value() && offset < *_committed)
            return {};

        // _entries отсортирован по offset, поэтому ищем бинарным поиском
        const auto it = std::ranges::lower_bound(_entries, offset, {}, &Entry::offset);
        if (it == _entries.end() || it->offset != offset)
            return std::unexpected(OffsetTrackerError::UnknownOffset);

        if (it->done)
            return {};

        it->done = true;
        --_inFlight;
        advancePrefix();
        return {};
    }

    // Позиция для коммита (оффсет СЛЕДУЮЩЕЙ записи) или nullopt, если коммитить нечего.
    // Состояние не меняет.
    [[nodiscard]] std::optional<std::int64_t> commitCandidate() const {
        if (_prefixDone == 0)
            return std::nullopt;
        return _entries[_prefixDone - 1].offset + 1;
    }

    // Брокер принял коммит этой позиции.
    [[nodiscard]] std::expected<void, std::error_code> confirmCommit(std::int64_t position) {
        if (_committed.has_value() && position <= *_committed)
            return {};

        if (const auto watermark = commitCandidate(); !watermark.has_value() || position > *watermark)
            return std::unexpected(OffsetTrackerError::CommitBeyondWatermark);

        while (!_entries.empty() && _entries.front().offset < position) {
            _entries.pop_front();
            --_prefixDone;
        }
        _committed = position;
        return {};
    }

    // Записи, не покрытые коммитом
    std::size_t uncommittedCount() const { return _entries.size(); }

    // Записи, которые ещё обрабатываются
    std::size_t inFlightCount() const { return _inFlight; }

    // Партиция потеряна/переназначена - начинаем с чистого листа
    void reset() {
        _entries.clear();
        _prefixDone = 0;
        _inFlight = 0;
        _lastRegistered.reset();
        _committed.reset();
    }

private:
    struct Entry {
        std::int64_t offset;
        bool done;
    };

    // Сдвигаем границу: сколько записей с начала очереди подряд готовы
    void advancePrefix() {
        while (_prefixDone < _entries.size() && _entries[_prefixDone].done)
            ++_prefixDone;
    }

private:
    std::deque<Entry> _entries; // все записи, не покрытые коммитом, по возрастанию оффсета
    std::size_t _prefixDone = 0; // длина непрерывного готового префикса _entries
    std::size_t _inFlight = 0;
    std::optional<std::int64_t> _lastRegistered;
    std::optional<std::int64_t> _committed;
};
}

#endif //LIQUIDPETPROJECT_OFFSETTRACKER_H
