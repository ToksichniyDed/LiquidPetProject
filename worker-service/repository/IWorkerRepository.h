//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_IWORKERREPOSITORY_H
#define LIQUIDPETPROJECT_IWORKERREPOSITORY_H

#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

#include <models/OrderIds.h>

namespace worker_service::repository {

enum class RepositoryError { NotFound = 1, ConnectionFailure, Timeout, ConstraintViolation, SerializationFailure };

class RepositoryErrorCategory : public std::error_category {
   public:
    const char* name() const noexcept override { return "repository"; }

    std::string message(int ev) const override {
        switch (static_cast<RepositoryError>(ev)) {
            case RepositoryError::NotFound:
                return "not found";
            case RepositoryError::ConnectionFailure:
                return "connection failure";
            case RepositoryError::Timeout:
                return "timeout";
            case RepositoryError::ConstraintViolation:
                return "constraint violation failure";
            case RepositoryError::SerializationFailure:
                return "serialization failure";
            default:
                return "unknown repository error";
        }
    }
};

inline const RepositoryErrorCategory& repositoryErrorCategory() {
    static RepositoryErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(RepositoryError e) {
    return std::error_code{std::to_underlying(e), repositoryErrorCategory()};
}
}  // namespace worker_service::repository

namespace std {
template <>
struct is_error_code_enum<worker_service::repository::RepositoryError> : true_type {};
}  // namespace std

namespace worker_service::repository {

struct ReservationRecord
{
    shared::models::OutboxEventId eventId;
    shared::models::OrderId aggregateId;
    std::string eventType;
    std::string payload;
};

class IWorkerRepository {
   public:
    IWorkerRepository() = default;
    virtual ~IWorkerRepository() = default;

    // false (без ошибки) => событие дубликат, ничего не записано
    // true => dedup-запись и outbox-запись успешно
    [[nodiscard]] virtual std::expected<bool, std::error_code> recordReservationIfNew(const ReservationRecord& record) = 0;
};
}  // namespace worker_service::repository

#endif  // LIQUIDPETPROJECT_IWORKERREPOSITORY_H
