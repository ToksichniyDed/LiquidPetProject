//
// Created by DED on 17.09.2026.
//

#ifndef LIQUIDPETPROJECT_UPSTREAMCONFIGURATIONJSONMAPPER_H
#define LIQUIDPETPROJECT_UPSTREAMCONFIGURATIONJSONMAPPER_H

#include <json/Json.h>

#include <models/UpstreamConfiguration.h>
#include <keys/UpstreamConfigurationJsonKeys.h>

namespace shared::models2json_mapper {
using namespace shared::models;
using namespace json;
using namespace models2json_mapper::keys;

class UpstreamConfigurationJsonMapper {
public:
    static std::expected<UpstreamConfiguration, std::error_code> fromJson(const nlohmann::json& section) {
        auto port = JsonHelper::getValue<uint16_t>(section, UPSTREAM_CONFIGURATION_PORT);
        if (!port.has_value())
            return std::unexpected(port.error());

        auto host = JsonHelper::getValue<std::string>(section, UPSTREAM_CONFIGURATION_HOST)
                .and_then([](std::string value) {
                    return Host::create(std::move(value));
                });

        if (!host.has_value())
            return std::unexpected(host.error());

        return UpstreamConfiguration{.host = host.value(), .port = port.value()};
    }
};
}

#endif //LIQUIDPETPROJECT_UPSTREAMCONFIGURATIONJSONMAPPER_H
