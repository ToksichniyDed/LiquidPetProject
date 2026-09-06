//
// Created by DED on 06.09.2026.
//

#include <pqxx/pqxx>

#include "PostgresOutboxRepository.h"
#include "OutboxRepositoryQueries.h"

#include <logging/Logger.h>

namespace order_service::outbox {
    using namespace order_system::repository::queries;

    namespace {
        inline constexpr auto ID = "id";
        inline constexpr auto AGGREGATE_ID = "aggregate_id";
        inline constexpr auto EVENT_TYPE = "event_type";
        inline constexpr auto PAYLOAD = "payload";
    }

    class PostgresOutboxRepository::Impl {
    public:
        explicit Impl(const order_system::models::DatabaseConfiguration& config) : _connection(
            config.toConnectionString()) {

            _connection.prepare(SELECT_UNPUBLISHED_OUTBOX, SELECT_UNPUBLISHED_OUTBOX_SQL);
            _connection.prepare(MARK_OUTBOX_PUBLISHED, MARK_OUTBOX_PUBLISHED_SQL);

            SPDLOG_LOGGER_INFO(Logger::get("PostgresOutboxRepository"), "Database connection successfully!");
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOutboxRepository"), "Database name : {}", config.databaseName());
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOutboxRepository"), "Database host : {}", config.host());
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOutboxRepository"), "Database port : {}", config.port());
        }

    public:
        pqxx::connection _connection;
    };

    namespace {

        std::error_code mapException(const std::exception& e) {
            if (dynamic_cast<const pqxx::broken_connection*>(&e))
                return OutboxRepositoryError::ConnectionFailure;
            if (dynamic_cast<const pqxx::unique_violation*>(&e) ||
                dynamic_cast<const pqxx::foreign_key_violation*>(&e) ||
                dynamic_cast<const pqxx::check_violation*>(&e))
                return OutboxRepositoryError::ConstraintViolation;
            if (dynamic_cast<const pqxx::in_doubt_error*>(&e))
                return OutboxRepositoryError::Timeout;

            return OutboxRepositoryError::SerializationFailure;
        }

        OutboxEntry mapRow(const pqxx::row_ref& row) {
            return OutboxEntry{
                .id = row[ID].as<std::string>(),
                .aggregateId = row[AGGREGATE_ID].as<std::string>(),
                .eventType = row[EVENT_TYPE].as<std::string>(),
                .payload = row[PAYLOAD].as<std::string>()
            };
        }

    }

    PostgresOutboxRepository::PostgresOutboxRepository(
        const order_system::models::DatabaseConfiguration& config) : _impl(
        std::make_unique<Impl>(config)) {
    }

    PostgresOutboxRepository::~PostgresOutboxRepository() = default;

    std::expected<std::vector<OutboxEntry>, std::error_code> PostgresOutboxRepository::fetchUnpublished(
        const int limit) {
        try {
            pqxx::work work(_impl->_connection);

            const auto rows = work.exec(pqxx::prepped{SELECT_UNPUBLISHED_OUTBOX}, pqxx::params{limit});

            work.commit();

            std::vector<OutboxEntry> entries;
            entries.reserve(rows.size());
            for (const auto& row : rows) {
                entries.push_back(mapRow(row));
            }

            return entries;
        } catch (const std::exception& e) {
            return std::unexpected(mapException(e));
        }
    }

    std::expected<void, std::error_code> PostgresOutboxRepository::markAsPublished(const std::string& entryId) {
        try {
            pqxx::work work(_impl->_connection);

            work.exec(pqxx::prepped{MARK_OUTBOX_PUBLISHED}, pqxx::params{entryId});

            work.commit();

            return {};
        } catch (const std::exception& e) {
            return std::unexpected(mapException(e));
        }
    }
}
