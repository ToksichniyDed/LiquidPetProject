//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_POSTGRESOUTBOXREPOSITORY_H
#define LIQUIDPETPROJECT_POSTGRESOUTBOXREPOSITORY_H

#include "IOutboxRepository.h"
#include "DatabaseConfiguration.h"

namespace order_service::outbox {
    class PostgresOutboxRepository : public IOutboxRepository {
    public:
        explicit PostgresOutboxRepository(const order_system::models::DatabaseConfiguration& config);
        ~PostgresOutboxRepository() override;

        PostgresOutboxRepository(const PostgresOutboxRepository&) = delete;
        PostgresOutboxRepository& operator=(const PostgresOutboxRepository&) = delete;

        [[nodiscard]] std::expected<std::vector<OutboxEntry>, std::error_code> fetchUnpublished(
            int limit) override;
        [[nodiscard]] std::expected<void, std::error_code> markAsPublished(
            const std::string& entryId) override;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
}

#endif //LIQUIDPETPROJECT_POSTGRESOUTBOXREPOSITORY_H
