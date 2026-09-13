//
// Created by DED on 13.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKWORKERREPOSITORY_H
#define LIQUIDPETPROJECT_MOCKWORKERREPOSITORY_H

#include <gmock/gmock.h>

#include "IWorkerRepository.h"

namespace worker_service::repository {

class MockWorkerRepository : public IWorkerRepository {
   public:
    MOCK_METHOD((std::expected<bool, std::error_code>), recordReservationIfNew, (const ReservationRecord& record),
                (override));
};

}  // namespace worker_service::repository

#endif  // LIQUIDPETPROJECT_MOCKWORKERREPOSITORY_H
