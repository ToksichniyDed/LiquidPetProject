//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H
#define LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H

#include <expected>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <OrderIds.h>

namespace shared::outbox {
    enum class OutboxRepositoryError : std::int8_t {
        NotFound = 1,
        ConnectionFailure,
        Timeout,
        ConstraintViolation,
        SerializationFailure
    };

    class OutboxRepositoryErrorCategory : public std::error_category {
        public:
        const char* name() const noexcept override { return "outbox_repository"; }

        std::string message(int ev) const override {
            switch (static_cast<OutboxRepositoryError>(ev)) {
                case OutboxRepositoryError::NotFound:
                    return "not found";
                case OutboxRepositoryError::ConnectionFailure:
                    return "connection failure";
                case OutboxRepositoryError::Timeout:
                    return "timeout";
                case OutboxRepositoryError::ConstraintViolation:
                    return "constraint violation failure";
                case OutboxRepositoryError::SerializationFailure:
                    return "serialization failure";
                default:
                    return "unknown outbox repository error";
            }
        }
    };

    inline const OutboxRepositoryErrorCategory& outboxRepositoryErrorCategory() {
        static OutboxRepositoryErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(OutboxRepositoryError e) {
        return std::error_code{std::to_underlying(e), outboxRepositoryErrorCategory()};
    }
}

namespace std {
    template <>
    struct is_error_code_enum<shared::outbox::OutboxRepositoryError> : true_type {
    };
}

namespace shared::outbox {

    struct OutboxEntry {
        models::OutboxEventId id;
        std::string aggregateId;
        std::string eventType;
        std::string payload;
    };

    class IOutboxRepository {
    public:
        IOutboxRepository() = default;
        virtual ~IOutboxRepository() = default;

        [[nodiscard]] virtual std::expected<std::vector<OutboxEntry>, std::error_code> fetchUnpublished(
            int limit) = 0;
        [[nodiscard]] virtual std::expected<void, std::error_code> markAsPublished(
            const models::OutboxEventId& entryId) = 0;
    };
}

#endif //LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H
