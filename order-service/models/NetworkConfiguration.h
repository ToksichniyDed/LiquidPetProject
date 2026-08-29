//
// Created by DED on 23.08.2026.
//

#ifndef LIQUIDPETPROJECT_NETWORKCONFIGURATION_H
#define LIQUIDPETPROJECT_NETWORKCONFIGURATION_H

#include <expected>

#include <nlohmann/json.hpp>

#include <NetworkAddress.h>
#include <json/Json.h>

namespace order_system::models {
    struct NetworkConfiguration {
        NetworkAddress address;
        uint16_t port;

        static std::expected<NetworkConfiguration, std::error_code> fromJson(const nlohmann::json& section) {
            auto port = Json::JsonHelper::getValue<uint16_t>(section, "port");
            if (!port.has_value())
                return std::unexpected(port.error());

            auto networkAddress = Json::JsonHelper::getValue<std::string>(section, "address")
                    .and_then([](std::string address) {
                        return NetworkAddress::create(std::move(address));
                    });

            if (!networkAddress.has_value())
                return std::unexpected(networkAddress.error());

            return NetworkConfiguration{.address = networkAddress.value(), .port = port.value()};
        }
    };
}

#endif //LIQUIDPETPROJECT_NETWORKCONFIGURATION_H
