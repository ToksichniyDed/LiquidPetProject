//
// Created by DED on 15.08.2026.
//

#include <filesystem>
#include <iostream>

#include <http/NetworkConfiguration.h>
#include <http/Route.h>
#include <http/HttpServer.h>
#include "handlers/RoutePaths.h"
#include <logging/Logger.h>
#include <CQRS/CreateOrderHandler.h>
#include <CQRS/GetOrderHandler.h>
#include <CQRS/HealthHandler.h>
#include "PostgresOrderRepository.h"
#include <http/NetworkConfigurationJsonMapper.h>
#include <mapper/DatabaseConfigurationJsonMapper.h>

int main(const int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>" << '\n';
        return 1;
    }

    Logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    std::filesystem::path configPath{argv[1]};

    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "DB_PASSWORD environment variable is not set");
        return 1;
    }

    auto networkSection = Json::JsonHelper::loadSection(configPath, "network");
    if (!networkSection.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n", networkSection.error().category().name(),
                               networkSection.error().message());
        return 1;
    }
    auto databaseSection = Json::JsonHelper::loadSection(configPath, "database");
    if (!databaseSection.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n", databaseSection.error().category().name(),
                               databaseSection.error().message());
        return 1;
    }

    auto networkConfiguration = shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
        networkSection.value());
    if (!networkConfiguration.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n", networkConfiguration.error().category().name(),
                               networkConfiguration.error().message());
        return 1;
    }

    auto databaseConfiguration = order_system::models2json_mapper::DatabaseConfigurationJsonMapper::fromJson(
        databaseSection.value(), dbPassword);
    if (!databaseConfiguration.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n", databaseConfiguration.error().category().name(),
                               databaseConfiguration.
                               error().message());
        return 1;
    }

    std::shared_ptr<order_system::repository::IOrderRepository> repository;
    try {
        repository = std::make_shared<
            order_system::repository::PostgresOrderRepository>(databaseConfiguration.value());
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error: {} \n", e.what());
        return 1;

    }


    std::vector<shared::http::handlers::Route> routes{
        {
            .method = shared::http::Method::Post,
            .pathPrefix = order_service::handlers::paths::ORDERS,
            .handler = std::make_shared<order_service::handlers::CreateOrderHandler>(repository)
        },
        {
            .method = shared::http::Method::Get,
            .pathPrefix = order_service::handlers::paths::ORDERS_PREFIX,
            .handler = std::make_shared<order_service::handlers::GetOrderHandler>(repository)
        },
        {
            .method = shared::http::Method::Get,
            .pathPrefix = order_service::handlers::paths::HEALTH,
            .handler = std::make_shared<order_service::handlers::HealthHandler>()
        },
    };

    order_service::handlers::HttpServer server{std::move(networkConfiguration.value()), routes};
    server.run();

    return 0;
}
