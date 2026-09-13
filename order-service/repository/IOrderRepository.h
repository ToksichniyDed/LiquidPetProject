//
// Created by DED on 18.08.2026.
//

#ifndef LIQUIDPETPROJECT_IORDERREPOSITORY_H
#define LIQUIDPETPROJECT_IORDERREPOSITORY_H

#include <Order.h>
#include <repository/RepositoryError.h>

#include <expected>
#include <system_error>

namespace order_system::repository {

using RepositoryError = shared::repository::RepositoryError;

class IOrderRepository {
   public:
    IOrderRepository() = default;
    virtual ~IOrderRepository() = default;

    [[nodiscard]] virtual std::expected<models::OrderId, std::error_code> save(const models::Order& order) = 0;
    [[nodiscard]] virtual std::expected<models::Order, std::error_code> findById(const models::OrderId& id) = 0;
};
}  // namespace order_system::repository

#endif  // LIQUIDPETPROJECT_IORDERREPOSITORY_H
