//
// Created by DED on 06.09.2026.
//

#include "PostgresOutboxRepository.h"

#include <logging/Logger.h>

#include <pqxx/pqxx>

#include "OutboxRepositoryQueries.h"
#include "models/OrderIds.h"

namespace shared::outbox {
using namespace queries;

namespace {
inline constexpr auto ID = "id";
inline constexpr auto AGGREGATE_ID = "aggregate_id";
inline constexpr auto EVENT_TYPE = "event_type";
inline constexpr auto PAYLOAD = "payload";
}  // namespace

class PostgresOutboxRepository::Impl {
   public:
    explicit Impl(const models::DatabaseConfiguration& config) : _connection(config.toConnectionString()) {
        _connection.prepare(SELECT_UNPUBLISHED_OUTBOX, SELECT_UNPUBLISHED_OUTBOX_SQL);
        _connection.prepare(MARK_OUTBOX_PUBLISHED, MARK_OUTBOX_PUBLISHED_SQL);

        SPDLOG_LOGGER_INFO(shared::logger::get("PostgresOutboxRepository"), "Database connection successfully!");
        SPDLOG_LOGGER_INFO(shared::logger::get("PostgresOutboxRepository"), "Database name : {}",
                           config.databaseName());
        SPDLOG_LOGGER_INFO(shared::logger::get("PostgresOutboxRepository"), "Database host : {}", config.host());
        SPDLOG_LOGGER_INFO(shared::logger::get("PostgresOutboxRepository"), "Database port : {}", config.port());
    }

   public:
    pqxx::connection _connection;
};

namespace {

std::error_code mapException(const std::exception& e) {
    using enum OutboxRepositoryError;
    if (dynamic_cast<const pqxx::broken_connection*>(&e)) return make_error_code(ConnectionFailure);
    if (dynamic_cast<const pqxx::unique_violation*>(&e) || dynamic_cast<const pqxx::foreign_key_violation*>(&e) ||
        dynamic_cast<const pqxx::check_violation*>(&e))
        return make_error_code(ConstraintViolation);
    if (dynamic_cast<const pqxx::in_doubt_error*>(&e)) return make_error_code(Timeout);

    return make_error_code(SerializationFailure);
}

std::expected<OutboxEntry, std::error_code> mapRow(const pqxx::row_ref& row) {
    auto idResult = models::OutboxEventId::create(row[ID].as<std::string>());
    if (!idResult.has_value()) {
        return std::unexpected(idResult.error());
    }

    return OutboxEntry{.id = std::move(idResult.value()),
                       .aggregateId = row[AGGREGATE_ID].as<std::string>(),
                       .eventType = row[EVENT_TYPE].as<std::string>(),
                       .payload = row[PAYLOAD].as<std::string>()};
}

}  // namespace

PostgresOutboxRepository::PostgresOutboxRepository(const models::DatabaseConfiguration& config)
    : _impl(std::make_unique<Impl>(config)) {}

PostgresOutboxRepository::~PostgresOutboxRepository() = default;

std::expected<std::vector<OutboxEntry>, std::error_code> PostgresOutboxRepository::fetchUnpublished(const int limit) {
    try {
        pqxx::work work(_impl->_connection);

        const auto rows = work.exec(pqxx::prepped{SELECT_UNPUBLISHED_OUTBOX}, pqxx::params{limit});

        work.commit();

        std::vector<OutboxEntry> entries;
        entries.reserve(rows.size());
        for (const auto& row : rows) {
            auto entryResult = mapRow(row);
            if (!entryResult.has_value()) {
                SPDLOG_LOGGER_ERROR(shared::logger::get("PostgresOutboxRepository"),
                                    "Failed to map outbox row: {}", entryResult.error().message());
                return std::unexpected(entryResult.error());
            }
            entries.push_back(std::move(entryResult.value()));
        }

        return entries;
    } catch (const std::exception& e) {
        return std::unexpected(mapException(e));
    }
}

std::expected<void, std::error_code> PostgresOutboxRepository::markAsPublished(const models::OutboxEventId& entryId) {
    try {
        pqxx::work work(_impl->_connection);

        work.exec(pqxx::prepped{MARK_OUTBOX_PUBLISHED}, pqxx::params{entryId.value()});

        work.commit();

        return {};
    } catch (const std::exception& e) {
        return std::unexpected(mapException(e));
    }
}
}  // namespace shared::outbox
