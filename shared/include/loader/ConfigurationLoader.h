//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_CONFIGURATIONLOADER_H
#define LIQUIDPETPROJECT_CONFIGURATIONLOADER_H

#include <expected>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace Loader {
    enum class ConfigurationParseError {
        InvalidFormat = 1,
        InvalidPath,
        FileAccessDenied,
        SectionNotFound,
        KeyNotFound,
        UnknownError
    };

    class ConfigurationParseErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "configuration parse"; }

        std::string message(int ev) const override {
            switch (static_cast<ConfigurationParseError>(ev)) {
                case ConfigurationParseError::InvalidFormat:
                    return "invalid configuration format";
                case ConfigurationParseError::InvalidPath:
                    return "invalid path";
                case ConfigurationParseError::FileAccessDenied:
                    return "file access denied";
                case ConfigurationParseError::SectionNotFound:
                    return "section not found";
                case ConfigurationParseError::KeyNotFound:
                    return "key not found";
                case ConfigurationParseError::UnknownError:
                    return "unknown error";
                default:
                    return "unknown configuration parse error";
            }
        };
    };

    inline ConfigurationParseErrorCategory& configurationParseErrorCategory() {
        static ConfigurationParseErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(ConfigurationParseError e) {
        return {static_cast<int>(e), configurationParseErrorCategory()};
    }
}

namespace std {
    template <>
    struct is_error_code_enum<Loader::ConfigurationParseError> : std::true_type {
    };
};

namespace Loader {
    class ConfigurationLoader {
    public:
        static std::expected<nlohmann::json, std::error_code> loadSection(
            const std::filesystem::path& path, const std::string& section);
        template <typename T>
        static std::expected<T, std::error_code> getValue(const nlohmann::json& object, const std::string& key);
    };

    template <typename T>
    std::expected<T, std::error_code> ConfigurationLoader::getValue(const nlohmann::json& object,
                                                                    const std::string& key) {
        try {
            return object.at(key).get<T>();
        } catch (const nlohmann::json::out_of_range& e) {
            return std::unexpected(ConfigurationParseError::KeyNotFound);
        } catch (const nlohmann::json::type_error& e) {
            return std::unexpected(ConfigurationParseError::InvalidFormat);
        } catch (...) {
            return std::unexpected(ConfigurationParseError::UnknownError);
        }
    }
}

#endif //LIQUIDPETPROJECT_CONFIGURATIONLOADER_H
