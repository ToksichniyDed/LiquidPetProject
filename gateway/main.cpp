//
// Created by DED on 03.09.2026.
//

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <CLI/CLI.hpp>

#include <models2json-mapper/mapper/NetworkConfigurationJsonMapper.h>
#include <models2json-mapper/mapper/UpstreamConfigurationJsonMapper.h>
#include <models/EnvironmentConfiguration.h>
#include <models/Host.h>
#include <http/Route.h>
#include <http/HttpServer.h>
#include <logging/Logger.h>

#include "handlers/HealthHandler.h"
#include "handlers/ProxyHandler.h"

namespace {

    struct CliOptions {
        std::filesystem::path configPath;
        std::optional<std::string> addressOverride;
        std::optional<std::uint16_t> portOverride;
    };

    void setupCliOptions(CLI::App& app, CliOptions& options) {
        app.add_option("-c,--config", options.configPath, "Path to config file")
           ->required()
           ->check(CLI::ExistingFile);
        app.add_option("--address", options.addressOverride, "Override gateway network address from config");
        app.add_option("--port", options.portOverride, "Override gateway network port from config");
    }

    CliOptions parseCliOptions(int argc, char* argv[]) {
        CLI::App app{"Gateway"};
        CliOptions options;
        setupCliOptions(app, options);

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            std::exit(app.exit(e));
        }

        return options;
    }

    template <typename T>
    T unwrapOrExit(std::expected<T, std::error_code> result) {
        if (!result.has_value()) {
            SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error {} : {}",
                                   result.error().category().name(), result.error().message());
            std::exit(1);
        }
        return std::move(result.value());
    }

    // Схема переменных окружения gateway. Все переменные опциональны - при их
    // отсутствии конфигурация полностью берётся из JSON-файла (--config)
    shared::models::EnvironmentSchema gatewayEnvironmentSchema() {
        return {
            {.name = "GATEWAY_ORDER_SERVICE_ADDRESS", .required = false},
            {.name = "GATEWAY_ORDER_SERVICE_PORT", .required = false},
        };
    }

    shared::models::NetworkConfiguration loadGatewayNetworkConfiguration(
        const std::filesystem::path& configPath,
        const std::optional<std::string>& addressOverride,
        const std::optional<std::uint16_t>& portOverride) {

        auto networkSection = shared::json::JsonHelper::loadSection(configPath, "network");
        auto networkConfiguration = unwrapOrExit(
            shared::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
                unwrapOrExit(std::move(networkSection))));

        if (addressOverride.has_value()) {
            auto overriddenAddress = unwrapOrExit(
                shared::models::NetworkAddress::create(*addressOverride));
            networkConfiguration.address = std::move(overriddenAddress);
            SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Gateway address overridden via CLI: {}", *addressOverride);
        }

        if (portOverride.has_value()) {
            networkConfiguration.port = *portOverride;
            SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Gateway port overridden via CLI: {}", *portOverride);
        }

        return networkConfiguration;
    }

    shared::models::UpstreamConfiguration loadOrderServiceConfiguration(
        const std::filesystem::path& configPath, const shared::models::EnvironmentConfiguration& environment) {

        auto envAddress = environment.get("GATEWAY_ORDER_SERVICE_ADDRESS");
        auto envPortRaw = environment.get("GATEWAY_ORDER_SERVICE_PORT");

        if (envAddress.has_value() && envPortRaw.has_value()) {
            auto host = unwrapOrExit(shared::models::Host::create(*envAddress));

            std::uint16_t port{};
            try {
                port = static_cast<std::uint16_t>(std::stoul(*envPortRaw));
            } catch (const std::exception&) {
                SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"),
                                       "GATEWAY_ORDER_SERVICE_PORT is not a valid port number: {}", *envPortRaw);
                std::exit(1);
            }

            SPDLOG_LOGGER_INFO(shared::logger::get("main"),
                               "order-service address overridden via environment: {}:{}", *envAddress, port);

            return shared::models::UpstreamConfiguration{.host = std::move(host), .port = port};
        }

        if (envAddress.has_value() != envPortRaw.has_value()) {
            // Задан только один из двух
            SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"),
                                   "GATEWAY_ORDER_SERVICE_ADDRESS and GATEWAY_ORDER_SERVICE_PORT must be set together, "
                                   "only one of them is present");
            std::exit(1);
        }

        auto servicesSection = unwrapOrExit(shared::json::JsonHelper::loadSection(configPath, "services"));

        if (!servicesSection.contains("orderService")) {
            SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Missing 'orderService' section under 'services'");
            std::exit(1);
        }

        return unwrapOrExit(
            shared::models2json_mapper::UpstreamConfigurationJsonMapper::fromJson(
                servicesSection["orderService"]));
    }

    std::vector<shared::http::handlers::Route> buildRoutes(
        const std::shared_ptr<gateway_service::handlers::ProxyHandler>& proxyHandler) {

        return {
            {.method=shared::models::Method::Get, .pathPrefix="/health",
             .handler=std::make_shared<gateway_service::handlers::HealthHandler>()},
            {.method=shared::models::Method::Get, .pathPrefix="/", .handler=proxyHandler},
            {.method=shared::models::Method::Post, .pathPrefix="/", .handler=proxyHandler},
        };
    }

}

int main(const int argc, char* argv[]) {
    const auto options = parseCliOptions(argc, argv);

    shared::logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    auto environment = unwrapOrExit(shared::models::EnvironmentConfiguration::load(gatewayEnvironmentSchema()));

    auto gatewayConfiguration = loadGatewayNetworkConfiguration(options.configPath, options.addressOverride, options.portOverride);

    auto orderServiceConfiguration = loadOrderServiceConfiguration(options.configPath, environment);

    std::unordered_map<std::string, shared::models::UpstreamConfiguration> services;
    services.emplace("/orders", std::move(orderServiceConfiguration));

    auto proxyHandler = std::make_shared<gateway_service::handlers::ProxyHandler>(std::move(services));

    shared::http::HttpServer server{std::move(gatewayConfiguration), buildRoutes(proxyHandler)};
    server.run();

    return 0;
}
