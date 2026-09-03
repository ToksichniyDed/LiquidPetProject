//
// Created by DED on 03.09.2026.
//

#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#include <http/NetworkConfigurationJsonMapper.h>
#include <http/Route.h>
#include <http/HttpServer.h>
#include <logging/Logger.h>

#include "handlers/ProxyHandler.h"

int main(const int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>" << '\n';
        return 1;
    }

    Logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    std::filesystem::path configPath{argv[1]};

    auto networkSection = Json::JsonHelper::loadSection(configPath, "network");
    if (!networkSection.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n",
                               networkSection.error().category().name(), networkSection.error().message());
        return 1;
    }

    auto servicesSection = Json::JsonHelper::loadSection(configPath, "services");
    if (!servicesSection.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n",
                               servicesSection.error().category().name(), servicesSection.error().message());
        return 1;
    }

    auto networkConfiguration = shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
        networkSection.value());
    if (!networkConfiguration.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n",
                               networkConfiguration.error().category().name(), networkConfiguration.error().message());
        return 1;
    }

    if (!servicesSection.value().contains("orderService")) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Missing 'orderService' section under 'services'\n");
        return 1;
    }

    auto orderServiceConfiguration = shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
        servicesSection.value()["orderService"]);
    if (!orderServiceConfiguration.has_value()) {
        SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {} \n",
                               orderServiceConfiguration.error().category().name(),
                               orderServiceConfiguration.error().message());
        return 1;
    }
    //TODO:временно записал сервис вручную
    std::unordered_map<std::string, shared::http::models::NetworkConfiguration> services;
    services.emplace("/orders", std::move(orderServiceConfiguration.value()));

    auto proxyHandler = std::make_shared<gateway_service::handlers::ProxyHandler>(std::move(services));

    std::vector<shared::http::handlers::Route> routes = {
        {shared::http::Method::Get, "/", proxyHandler},
        {shared::http::Method::Post, "/", proxyHandler},
    };

    shared::http::HttpServer server{std::move(networkConfiguration.value()), routes};
    server.run();

    return 0;
}
