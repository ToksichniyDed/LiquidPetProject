//
// Created by DED on 31.08.2026.
//

#ifndef LIQUIDPETPROJECT_DATABASECONFIGURATIONJSONMAPPER_H
#define LIQUIDPETPROJECT_DATABASECONFIGURATIONJSONMAPPER_H

#include <keys/DatabaseConfigurationJsonKeys.h>
#include "DatabaseConfiguration.h"
#include <json/Json.h>

namespace order_system::models2json_mapper {
    using namespace order_system::models;
    using namespace Json;
    using namespace models2json_mapper::keys;

    class DatabaseConfigurationJsonMapper {
    public:
        static std::expected<DatabaseConfiguration, std::error_code> fromJson(
            const nlohmann::json& json, std::string password) {

            auto host = JsonHelper::getValue<std::string>(json, DATABASE_HOST);
            if (!host.has_value())
                return std::unexpected(host.error());

            auto port = JsonHelper::getValue<std::uint16_t>(json, DATABASE_PORT);
            if (!port.has_value())
                return std::unexpected(port.error());

            auto databaseName = JsonHelper::getValue<std::string>(json, DATABASE_NAME);
            if (!databaseName.has_value())
                return std::unexpected(databaseName.error());

            auto user = JsonHelper::getValue<std::string>(json, DATABASE_USER);
            if (!user.has_value())
                return std::unexpected(user.error());

            auto useSsl = JsonHelper::getValue<bool>(json, DATABASE_USE_SSL);
            if (!useSsl.has_value())
                return std::unexpected(useSsl.error());

            return DatabaseConfiguration::create(
                std::move(host.value()), port.value(), std::move(databaseName.value()),
                std::move(user.value()), std::move(password), useSsl.value()
            );
        }

        static nlohmann::json toJson(const DatabaseConfiguration& config) {
            nlohmann::json json;
            json[DATABASE_HOST] = config.host();
            json[DATABASE_PORT] = config.port();
            json[DATABASE_NAME] = config.databaseName();
            json[DATABASE_USER] = config.user();
            json[DATABASE_USE_SSL] = config.useSsl();
            return json;
        }
    };
}

#endif //LIQUIDPETPROJECT_DATABASECONFIGURATIONJSONMAPPER_H
