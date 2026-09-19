//
// Created by DED on 19.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKORDERSTATUSREPOSITORY_H
#define LIQUIDPETPROJECT_MOCKORDERSTATUSREPOSITORY_H

#include <gmock/gmock.h>

#include "IOrderStatusRepository.h"

namespace order_system::repository {

    class MockOrderStatusRepository : public IOrderStatusRepository {
    public:
        MOCK_METHOD((std::expected<bool, std::error_code>), changeStatus,
                    (const models::OrderId& id, models::Order::OrderStatus fromStatus,
                     models::Order::OrderStatus toStatus),
                    (override));
    };

}

#endif //LIQUIDPETPROJECT_MOCKORDERSTATUSREPOSITORY_H
