//
// Created by DED on 19.09.2026.
//

#ifndef LIQUIDPETPROJECT_IORDERSTATUSREPOSITORY_H
#define LIQUIDPETPROJECT_IORDERSTATUSREPOSITORY_H

#include <Order.h>

#include <expected>
#include <system_error>

namespace order_system::repository {

    class IOrderStatusRepository {
    public:
        IOrderStatusRepository() = default;
        virtual ~IOrderStatusRepository() = default;

        // Атомарно меняет статус, только если текущий статус в БД равен fromStatus (compare-and-set).
        // true  => статус изменён
        // false => ничего не изменено: заказа нет, либо статус уже другой (его поменял кто-то параллельно)
        [[nodiscard]] virtual std::expected<bool, std::error_code> changeStatus(
            const models::OrderId& id, models::Order::OrderStatus fromStatus, models::Order::OrderStatus toStatus) = 0;
    };

}

#endif //LIQUIDPETPROJECT_IORDERSTATUSREPOSITORY_H
