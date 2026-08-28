//
// Created by DED on 15.08.2026.
//

#include <filesystem>
#include <iostream>

#include <json/JsonHelper.h>
#include <NetworkConfiguration.h>

#include "handlers/HttpServer.h"
#include "logging/Logger.h"

int main(const int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }

    std::filesystem::path configPath{argv[1]};

    auto networkSection = Loader::JsonHelper::loadSection(configPath, "network");
    if (!networkSection.has_value()) {
        std::cerr << "Error "<< networkSection.error().category().name()<< ": " << networkSection.error().message() << '\n';
        return 1;
    }

    auto networkConfiguration = order_system::models::NetworkConfiguration::fromJson(networkSection.value());
    if (!networkConfiguration.has_value()) {
        std::cerr << "Error "<< networkConfiguration.error().category().name()<< ": " << networkConfiguration.error().message() << '\n';
        return 1;
    }

    Logger::init(true, false, spdlog::level::level_enum::debug, {},1024, 0);
    order_service::handlers::HttpServer server{std::move(networkConfiguration.value())};
    server.run();

    return 0;
}
