//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H
#define LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H

#include <OrderIds.h>
#include <repository/RepositoryError.h>

#include <expected>
#include <string>
#include <system_error>
#include <vector>

namespace shared::outbox {

    using OutboxRepositoryError = shared::repository::RepositoryError;

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
