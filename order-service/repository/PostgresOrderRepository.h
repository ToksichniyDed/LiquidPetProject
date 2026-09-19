//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H
#define LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H

#include <models/DatabaseConfiguration.h>

#include "IOrderRepository.h"
#include "IOrderStatusRepository.h"

namespace order_system::repository {
    class PostgresOrderRepository : public IOrderRepository , public IOrderStatusRepository  {
    public:
        explicit PostgresOrderRepository(const shared::models::DatabaseConfiguration& config);
        ~PostgresOrderRepository() override;

        PostgresOrderRepository(const PostgresOrderRepository&) = delete;
        PostgresOrderRepository& operator=(const PostgresOrderRepository&) = delete;

        std::expected<models::OrderId, std::error_code> save(const models::Order& order) override;
        std::expected<models::Order, std::error_code> findById(const models::OrderId& id) override;
        std::expected<bool, std::error_code> changeStatus(const models::OrderId& id,
                                                          models::Order::OrderStatus fromStatus,
                                                          models::Order::OrderStatus toStatus) override;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}


#endif //LIQUIDPETPROJECT_POSTGRESORDERREPOSITORY_H
