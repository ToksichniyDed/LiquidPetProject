//
// Created by DED on 20.08.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKORDERREPOSITORY_H
#define LIQUIDPETPROJECT_MOCKORDERREPOSITORY_H

#include <gmock/gmock.h>

#include "IOrderRepository.h"

namespace order_system::repository {

    class MockOrderRepository : public IOrderRepository {
    public:
        MOCK_METHOD((std::expected<models::OrderId, std::error_code>), save,
            (const models::Order& order), (override));
        MOCK_METHOD((std::expected<models::Order, std::error_code>), findById,
                    (const models::OrderId& id), (override));
    };
}

#endif //LIQUIDPETPROJECT_MOCKORDERREPOSITORY_H
