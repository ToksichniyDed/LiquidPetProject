//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H
#define LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H

#include <DatabaseConfiguration.h>
#include "IOrderRepository.h"

namespace order_system::repository {
    class PostgresOrderRepository : public IOrderRepository {
    public:
        explicit PostgresOrderRepository(const models::DatabaseConfiguration& config);
        ~PostgresOrderRepository() override;

        PostgresOrderRepository(const PostgresOrderRepository&) = delete;
        PostgresOrderRepository& operator=(const PostgresOrderRepository&) = delete;

        std::expected<models::OrderId, std::error_code> save(const models::Order& order) override;
        std::expected<models::Order, std::error_code> findById(const models::OrderId& id) override;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}


#endif //LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H
