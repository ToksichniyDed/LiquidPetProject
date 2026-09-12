//
// Created by DED on 10.09.2026.
//

#include "PostgresWorkerRepository.h"

#include <logging/Logger.h>

#include <pqxx/pqxx>

#include "WorkerRepositoryQueries.h"

namespace worker_service::repository {
using namespace shared::logger;

class PostgresWorkerRepository::Impl {
   public:
    explicit Impl(const shared::models::DatabaseConfiguration& configuration)
        : _connection(configuration.toConnectionString()) {
        _connection.prepare(queries::INSERT_PROCESSED_EVENT, queries::INSERT_PROCESSED_EVENT_SQL);
        _connection.prepare(queries::INSERT_OUTBOX_EVENT, queries::INSERT_OUTBOX_EVENT_SQL);

        SPDLOG_LOGGER_INFO(get("PostgresWorkerRepository"), "Database connection successfully!");
        SPDLOG_LOGGER_INFO(get("PostgresWorkerRepository"), "Database name : {}", configuration.databaseName());
        SPDLOG_LOGGER_INFO(get("PostgresWorkerRepository"), "Database host : {}", configuration.host());
        SPDLOG_LOGGER_INFO(get("PostgresWorkerRepository"), "Database port : {}", configuration.port());
        SPDLOG_LOGGER_INFO(get("PostgresWorkerRepository"), "Database user : {}", configuration.user());
    }

   public:
    pqxx::connection _connection;
};

namespace {

std::error_code mapException(const std::exception& e) {
    using enum RepositoryError;
    if (dynamic_cast<const pqxx::broken_connection*>(&e)) return ConnectionFailure;
    if (dynamic_cast<const pqxx::unique_violation*>(&e) || dynamic_cast<const pqxx::foreign_key_violation*>(&e) ||
        dynamic_cast<const pqxx::check_violation*>(&e))
        return ConstraintViolation;
    if (dynamic_cast<const pqxx::in_doubt_error*>(&e)) return Timeout;

    return SerializationFailure;
}

}  // namespace

PostgresWorkerRepository::PostgresWorkerRepository(const shared::models::DatabaseConfiguration& configuration)
    : _impl(std::make_unique<Impl>(configuration)) {}

PostgresWorkerRepository::~PostgresWorkerRepository() = default;

std::expected<bool, std::error_code> PostgresWorkerRepository::recordReservationIfNew(const ReservationRecord& record) {
    try {
        pqxx::work work(_impl->_connection);

        if (auto processResult =
                work.exec(pqxx::prepped{queries::INSERT_PROCESSED_EVENT}, pqxx::params{record.eventId.value()});
            processResult.affected_rows() == 0) {
            work.abort();
            return false;
        }

        auto outboxResult = work.exec(pqxx::prepped{queries::INSERT_OUTBOX_EVENT},
                                      pqxx::params{record.aggregateId.value(), record.eventType, record.payload});

        work.commit();

        return true;
    } catch (const std::exception& e) {
        return std::unexpected(mapException(e));
    }
    return true;
}

}  // namespace worker_service::repository
