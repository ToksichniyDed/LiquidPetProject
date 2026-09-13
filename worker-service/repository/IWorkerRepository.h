//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_IWORKERREPOSITORY_H
#define LIQUIDPETPROJECT_IWORKERREPOSITORY_H

#include <models/OrderIds.h>
#include <repository/RepositoryError.h>

#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

namespace worker_service::repository {

struct ReservationRecord
{
    shared::models::OutboxEventId eventId;
    shared::models::OrderId aggregateId;
    std::string eventType;
    std::string payload;
};

class IWorkerRepository {

    using RepositoryError = shared::repository::RepositoryError;

   public:
    IWorkerRepository() = default;
    virtual ~IWorkerRepository() = default;

    // false (без ошибки) => событие дубликат, ничего не записано
    // true => dedup-запись и outbox-запись успешно
    [[nodiscard]] virtual std::expected<bool, std::error_code> recordReservationIfNew(const ReservationRecord& record) = 0;
};
}  // namespace worker_service::repository

#endif  // LIQUIDPETPROJECT_IWORKERREPOSITORY_H
