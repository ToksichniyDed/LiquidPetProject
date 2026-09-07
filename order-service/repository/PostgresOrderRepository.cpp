//
// Created by DED on 30.08.2026.
//

#include <pqxx/pqxx>

#include "PostgresOrderRepository.h"
#include "OrderRepositoryQueries.h"
#include "mapper/OrderRowMapper.h"
#include "mapper/OrderJsonMapper.h"

#include <json/Json.h>
#include <logging/Logger.h>

namespace order_system::repository {
    using namespace order_system::models;
    using namespace order_system::repository::queries;
    using namespace order_system::repository::columns;

    class PostgresOrderRepository::Impl {
    public:
        explicit Impl(const DatabaseConfiguration& config) : _connection(config.toConnectionString()) {

            _connection.prepare(INSERT_ORDER, INSERT_ORDER_SQL);
            _connection.prepare(INSERT_ORDER_ITEM, INSERT_ORDER_ITEM_SQL);
            _connection.prepare(INSERT_OUTBOX_EVENT, INSERT_OUTBOX_EVENT_SQL);
            _connection.prepare(SELECT_ORDER, SELECT_ORDER_SQL);
            _connection.prepare(SELECT_ORDER_ITEMS, SELECT_ORDER_ITEMS_SQL);

            SPDLOG_LOGGER_INFO(Logger::get("PostgresOrderRepository"), "Database connection successfully!");
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOrderRepository"), "Database name : {}", config.databaseName());
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOrderRepository"), "Database host : {}", config.host());
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOrderRepository"), "Database port : {}", config.port());
            SPDLOG_LOGGER_INFO(Logger::get("PostgresOrderRepository"), "Database user : {}", config.user());
        }

    public:
        pqxx::connection _connection;
    };

    namespace {

        std::error_code mapException(const std::exception& e) {
            if (dynamic_cast<const pqxx::broken_connection*>(&e))
                return RepositoryError::ConnectionFailure;
            if (dynamic_cast<const pqxx::unique_violation*>(&e) ||
                dynamic_cast<const pqxx::foreign_key_violation*>(&e) ||
                dynamic_cast<const pqxx::check_violation*>(&e))
                return RepositoryError::ConstraintViolation;
            if (dynamic_cast<const pqxx::in_doubt_error*>(&e))
                return RepositoryError::Timeout;

            return RepositoryError::SerializationFailure;
        }

    }

    PostgresOrderRepository::PostgresOrderRepository(const DatabaseConfiguration& config) : _impl(
        std::make_unique<Impl>(config)) {
    }

    PostgresOrderRepository::~PostgresOrderRepository() = default;

    std::expected<OrderId, std::error_code> PostgresOrderRepository::save(const Order& order) {
        try {
            pqxx::work work(_impl->_connection);

            auto orderResult = work.exec(pqxx::prepped{INSERT_ORDER}, pqxx::params{
                                             order.userId().value(),
                                             OrderStatusMapper::toString(order.status())
                                         }
            );

            const auto& orderIdValue = orderResult[0][ORDER_ID].as<std::string>();

            for (const auto& item : order.items()) {
                work.exec(pqxx::prepped{INSERT_ORDER_ITEM},
                          pqxx::params{
                              orderIdValue,
                              item.productId().value(),
                              std::to_string(item.quantity()),
                              std::to_string(item.priceAtOrderTime().minorUnits()),
                              item.priceAtOrderTime().currency().code(),
                              std::to_string(item.priceAtOrderTime().currency().minorDigits())
                          }
                );
            }

            auto orderId = OrderId::create(orderIdValue);
            if (!orderId.has_value()) {
                return std::unexpected(orderId.error());
            }

            auto orderWithId = order;
            orderWithId.assignId(*orderId);

            work.exec(pqxx::prepped{INSERT_OUTBOX_EVENT}, pqxx::params{
                          orderIdValue,
                          "OrderCreated",
                          models2json_mapper::OrderJsonMapper::toJson(orderWithId).dump()
                      });

            work.commit();

            return orderId;
        } catch (const std::exception& e) {
            return std::unexpected(mapException(e));
        }
    }

    std::expected<Order, std::error_code> PostgresOrderRepository::findById(const OrderId& id) {
        try {
            pqxx::work work(_impl->_connection);

            auto orderRows = work.exec(pqxx::prepped{SELECT_ORDER}, pqxx::params{id.value()});
            if (orderRows.empty())
                return std::unexpected(RepositoryError::NotFound);

            auto itemsRow = work.exec(pqxx::prepped{SELECT_ORDER_ITEMS}, pqxx::params{id.value()});

            work.commit();

            return OrderRowMapper::fromRows(orderRows.one_row(), itemsRow);
        } catch (const std::exception& e) {
            return std::unexpected(mapException(e));
        }
    }
}
