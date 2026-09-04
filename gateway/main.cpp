//
// Created by DED on 03.09.2026.
//

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <CLI/CLI.hpp>

#include <http/NetworkConfigurationJsonMapper.h>
#include <http/Route.h>
#include <http/HttpServer.h>
#include <logging/Logger.h>

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
            SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Error {} : {}",
                                   result.error().category().name(), result.error().message());
            std::exit(1);
        }
        return std::move(result.value());
    }

    shared::http::models::NetworkConfiguration loadGatewayNetworkConfiguration(
        const std::filesystem::path& configPath,
        const std::optional<std::string>& addressOverride,
        const std::optional<std::uint16_t>& portOverride) {

        auto networkSection = Json::JsonHelper::loadSection(configPath, "network");
        auto networkConfiguration = unwrapOrExit(
            shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
                unwrapOrExit(std::move(networkSection))));

        if (addressOverride.has_value()) {
            auto overriddenAddress = unwrapOrExit(
                shared::http::models::NetworkAddress::create(*addressOverride));
            networkConfiguration.address = overriddenAddress;
            SPDLOG_LOGGER_INFO(Logger::get("main"), "Gateway address overridden via CLI: {}", *addressOverride);
        }

        if (portOverride.has_value()) {
            networkConfiguration.port = *portOverride;
            SPDLOG_LOGGER_INFO(Logger::get("main"), "Gateway port overridden via CLI: {}", *portOverride);
        }

        return networkConfiguration;
    }

    shared::http::models::NetworkConfiguration loadOrderServiceConfiguration(const std::filesystem::path& configPath) {
        auto servicesSection = unwrapOrExit(Json::JsonHelper::loadSection(configPath, "services"));

        if (!servicesSection.contains("orderService")) {
            SPDLOG_LOGGER_CRITICAL(Logger::get("main"), "Missing 'orderService' section under 'services'");
            std::exit(1);
        }

        return unwrapOrExit(
            shared::http::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
                servicesSection["orderService"]));
    }

    std::vector<shared::http::handlers::Route> buildRoutes(
        const std::shared_ptr<gateway_service::handlers::ProxyHandler>& proxyHandler) {

        return {
            {shared::http::Method::Get, "/", proxyHandler},
            {shared::http::Method::Post, "/", proxyHandler},
        };
    }

}

int main(const int argc, char* argv[]) {
    const auto options = parseCliOptions(argc, argv);

    Logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    auto gatewayConfiguration = loadGatewayNetworkConfiguration(
        options.configPath, options.addressOverride, options.portOverride);

    auto orderServiceConfiguration = loadOrderServiceConfiguration(options.configPath);

    std::unordered_map<std::string, shared::http::models::NetworkConfiguration> services;
    services.emplace("/orders", std::move(orderServiceConfiguration));

    auto proxyHandler = std::make_shared<gateway_service::handlers::ProxyHandler>(std::move(services));

    shared::http::HttpServer server{std::move(gatewayConfiguration), buildRoutes(proxyHandler)};
    server.run();

    return 0;
}
