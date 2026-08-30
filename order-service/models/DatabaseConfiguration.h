//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_DATABASECONFIGURATION_H
#define LIQUIDPETPROJECT_DATABASECONFIGURATION_H

#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

namespace order_system::models {
    enum class DatabaseConfigurationError : std::uint8_t {
        EmptyHost = 1,
        EmptyDatabaseName,
        EmptyUser,
        InvalidPort
    };

    class DatabaseConfigurationErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "databaseConfiguration"; }

        std::string message(int ev) const override {
            switch (static_cast<DatabaseConfigurationError>(ev)) {
                case DatabaseConfigurationError::EmptyHost:
                    return "empty host";
                case DatabaseConfigurationError::EmptyDatabaseName:
                    return "empty database name";
                case DatabaseConfigurationError::EmptyUser:
                    return "empty user";
                case DatabaseConfigurationError::InvalidPort:
                    return "invalid port";
                default:
                    return "unknown database config error";
            }
        }
    };

    inline const DatabaseConfigurationErrorCategory& databaseConfigErrorCategory() {
        static DatabaseConfigurationErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(DatabaseConfigurationError e) {
        return {static_cast<int>(e), databaseConfigErrorCategory()};
    }
}

namespace std {
    template <>
    struct is_error_code_enum<order_system::models::DatabaseConfigurationError> : true_type {
    };
}

namespace order_system::models {

    class DatabaseConfiguration {
    public:
        static std::expected<DatabaseConfiguration, std::error_code> create(
            std::string host, std::uint16_t port, std::string databaseName,
            std::string user, std::string password, bool useSsl = false) {

            if (host.empty())
                return std::unexpected(DatabaseConfigurationError::EmptyHost);
            if (databaseName.empty())
                return std::unexpected(DatabaseConfigurationError::EmptyDatabaseName);
            if (user.empty())
                return std::unexpected(DatabaseConfigurationError::EmptyUser);
            if (port == 0)
                return std::unexpected(DatabaseConfigurationError::InvalidPort);

            return DatabaseConfiguration{
                std::move(host), port, std::move(databaseName),
                std::move(user), std::move(password), useSsl
            };
        }

        std::string toConnectionString() const {
            return "host=" + _host +
                   " port=" + std::to_string(_port) +
                   " dbname=" + _databaseName +
                   " user=" + _user +
                   " password=" + _password +
                   " sslmode=" + (_useSsl ? "require" : "disable");
        }

        const std::string& host() const { return _host; }
        std::uint16_t port() const { return _port; }
        const std::string& databaseName() const { return _databaseName; }
        const std::string& user() const { return _user; }
        bool useSsl() const { return _useSsl; }

    private:
        DatabaseConfiguration(std::string host, std::uint16_t port, std::string databaseName,
                              std::string user, std::string password, bool useSsl) : _host(std::move(host)), _port(port),
                                                                              _databaseName(std::move(databaseName)),
                                                                              _user(std::move(user)),
                                                                              _password(std::move(password)),
                                                                              _useSsl(useSsl) {
        }

    private:
        std::string _host;
        std::uint16_t _port;
        std::string _databaseName;
        std::string _user;
        std::string _password;
        bool _useSsl;
    };
}

#endif //LIQUIDPETPROJECT_DATABASECONFIGURATION_H
