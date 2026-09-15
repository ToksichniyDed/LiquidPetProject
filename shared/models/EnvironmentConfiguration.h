//
// Created by DED on 15.09.2026.
//

#ifndef LIQUIDPETPROJECT_ENVIRONMENTCONFIGURATION_H
#define LIQUIDPETPROJECT_ENVIRONMENTCONFIGURATION_H

#include <cstdlib>
#include <expected>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace shared::models {

enum class EnvironmentConfigurationError : std::uint8_t {
    MissingRequiredVariable = 1,
};

class EnvironmentConfigurationErrorCategory : public std::error_category {
   public:
    const char* name() const noexcept override { return "environmentConfiguration"; }

    std::string message(int ev) const override {
        switch (static_cast<EnvironmentConfigurationError>(ev)) {
            case EnvironmentConfigurationError::MissingRequiredVariable:
                return "missing required environment variable";
            default:
                return "unknown environment configuration error";
        }
    }
};

inline const EnvironmentConfigurationErrorCategory& environmentConfigurationErrorCategory() {
    static EnvironmentConfigurationErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(EnvironmentConfigurationError e) {
    return {static_cast<int>(e), environmentConfigurationErrorCategory()};
}

}  // namespace shared::models

namespace std {
template <>
struct is_error_code_enum<shared::models::EnvironmentConfigurationError> : true_type {};
}  // namespace std

namespace shared::models {

// Декларация одной переменной окружения, которую сервис готов прочитать.
// Схема сервиса - это просто список таких деклараций

struct EnvironmentVariableSpec {
    std::string name;
    bool required = true;
    std::optional<std::string> defaultValue = std::nullopt;
};

using EnvironmentSchema = std::vector<EnvironmentVariableSpec>;

class EnvironmentConfiguration {
   public:
    // Читает переменные окружения согласно schema. Для каждой required
    // переменной, отсутствующей в окружении, возвращает ошибку с первой
    // же найденной проблемой
    static std::expected<EnvironmentConfiguration, std::error_code> load(const EnvironmentSchema& schema) {
        std::unordered_map<std::string, std::string> values;

        for (const auto& spec : schema) {
            const char* raw = std::getenv(spec.name.c_str());

            if (raw != nullptr) {
                values[spec.name] = raw;
                continue;
            }

            if (spec.defaultValue.has_value()) {
                values[spec.name] = spec.defaultValue.value();
                continue;
            }

            if (spec.required) {
                return std::unexpected(std::error_code(EnvironmentConfigurationError::MissingRequiredVariable));
            }
        }

        return EnvironmentConfiguration{std::move(values)};
    }

    std::optional<std::string> get(const std::string& name) const {
        auto it = _values.find(name);
        if (it == _values.end()) return std::nullopt;
        return it->second;
    }

    // Удобный доступ для гарантированно присутствующих (required или с
    // дефолтом) переменных - вызывающий код сам решает, когда это уместно.
    const std::string& require(const std::string& name) const { return _values.at(name); }

   private:
    explicit EnvironmentConfiguration(std::unordered_map<std::string, std::string> values)
        : _values(std::move(values)) {}

   private:
    std::unordered_map<std::string, std::string> _values;
};

}  // namespace shared::models

#endif  // LIQUIDPETPROJECT_ENVIRONMENTCONFIGURATION_H
