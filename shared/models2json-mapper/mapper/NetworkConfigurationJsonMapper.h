//
// Created by DED on 31.08.2026.
//

#ifndef LIQUIDPETPROJECT_NETWORKJSONMAPPER_H
#define LIQUIDPETPROJECT_NETWORKJSONMAPPER_H

#include <json/Json.h>

#include <models/NetworkConfiguration.h>
#include <keys/NetworkConfigurationJsonKeys.h>

namespace shared::models2json_mapper {
    using namespace shared::models;
    using namespace json;
    using namespace models2json_mapper::keys;

    class NetworkConfigurationJsonMapper {
    public:
        static std::expected<NetworkConfiguration, std::error_code> fromJson(const nlohmann::json& section) {
            auto port = JsonHelper::getValue<uint16_t>(section, NETWORK_CONFIGURATION_PORT);
            if (!port.has_value())
                return std::unexpected(port.error());

            auto networkAddress = JsonHelper::getValue<std::string>(section, NETWORK_CONFIGURATION_ADDRESS)
                    .and_then([](std::string address) {
                        return NetworkAddress::create(std::move(address));
                    });

            if (!networkAddress.has_value())
                return std::unexpected(networkAddress.error());

            return NetworkConfiguration{.address = networkAddress.value(), .port = port.value()};
        }
    };
}


#endif //LIQUIDPETPROJECT_NETWORKJSONMAPPER_H
