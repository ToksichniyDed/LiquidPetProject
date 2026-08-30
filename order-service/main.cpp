//
// Created by DED on 15.08.2026.
//

#include <filesystem>
#include <iostream>

#include <json/Json.h>
#include <models/NetworkConfiguration.h>
#include <handlers/HttpServer.h>
#include <handlers/RoutePaths.h>
#include <logging/Logger.h>
#include "CQRS/CreateOrderHandler.h"
#include "CQRS/GetOrderHandler.h"
#include "CQRS/HealthHandler.h"
#include <repository/PostgresOrderRepository.h>
#include "mapper/NetworkConfigurationJsonMapper.h"
#include "mapper/DatabaseConfigurationJsonMapper.h"

int main(const int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>" << '\n';
        return 1;
    }

    std::filesystem::path configPath{argv[1]};

    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "DB_PASSWORD environment variable is not set");
        return 1;
    }

    auto networkSection = Json::JsonHelper::loadSection(configPath, "network");
    if (!networkSection.has_value()) {
        std::cerr << "Error "<< networkSection.error().category().name()<< ": " << networkSection.error().message() << '\n';
        return 1;
    }
    auto databaseSection = Json::JsonHelper::loadSection(configPath, "database");
    if (!databaseSection.has_value()) {
        std::cerr << "Error " << databaseSection.error().category().name() << ": " << databaseSection.error().message()
                << '\n';
        return 1;
    }

    auto networkConfiguration = order_system::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
        networkSection.value());
    if (!networkConfiguration.has_value()) {
        std::cerr << "Error "<< networkConfiguration.error().category().name()<< ": " << networkConfiguration.error().message() << '\n';
        return 1;
    }

    auto databaseConfiguration = order_system::models2json_mapper::DatabaseConfigurationJsonMapper::fromJson(
        databaseSection.value(), dbPassword);
    if (!databaseConfiguration.has_value()) {
        std::cerr << "Error " << databaseConfiguration.error().category().name() << ": " << databaseConfiguration.
                error().message() << '\n';
        return 1;
    }

    Logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    std::shared_ptr<order_system::repository::IOrderRepository> repository = std::make_shared<
        order_system::repository::PostgresOrderRepository>(databaseConfiguration.value());

    std::vector<order_service::handlers::Route> routes{
        {
            .method = Http::Method::Post,
            .pathPrefix = order_service::handlers::paths::ORDERS,
            .handler = std::make_shared<order_service::handlers::CreateOrderHandler>(repository)
        },
        {
            .method = Http::Method::Get,
            .pathPrefix = order_service::handlers::paths::ORDERS_PREFIX,
            .handler = std::make_shared<order_service::handlers::GetOrderHandler>(repository)
        },
        {
            .method = Http::Method::Get,
            .pathPrefix = order_service::handlers::paths::HEALTH,
            .handler = std::make_shared<order_service::handlers::HealthHandler>()
        },
    };

    order_service::handlers::HttpServer server{std::move(networkConfiguration.value()), routes};
    server.run();

    return 0;
}
