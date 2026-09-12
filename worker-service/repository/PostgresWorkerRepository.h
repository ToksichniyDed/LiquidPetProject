//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_POSTGRESWORKERREPOSITORY_H
#define LIQUIDPETPROJECT_POSTGRESWORKERREPOSITORY_H

#include "IWorkerRepository.h"

#include "http/HttpMessageConverter.h"
#include <models/DatabaseConfiguration.h>

namespace worker_service::repository {
class PostgresWorkerRepository : public IWorkerRepository {
    public:
    explicit PostgresWorkerRepository(const shared::models::DatabaseConfiguration& configuration);
    ~PostgresWorkerRepository() override;
    std::expected<bool, std::error_code> recordReservationIfNew(const ReservationRecord& record) override;

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}



#endif //LIQUIDPETPROJECT_POSTGRESWORKERREPOSITORY_H
