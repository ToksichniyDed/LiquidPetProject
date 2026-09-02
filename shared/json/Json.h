//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_CONFIGURATIONLOADER_H
#define LIQUIDPETPROJECT_CONFIGURATIONLOADER_H

#include <expected>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace Json {
    enum class JsonParseError {
        InvalidFormat = 1,
        InvalidPath,
        FileAccessDenied,
        SectionNotFound,
        KeyNotFound,
        UnknownError
    };

    class JsonParseErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "json parse"; }

        std::string message(int ev) const override {
            switch (static_cast<JsonParseError>(ev)) {
                case JsonParseError::InvalidFormat:
                    return "invalid json format";
                case JsonParseError::InvalidPath:
                    return "invalid path";
                case JsonParseError::FileAccessDenied:
                    return "file access denied";
                case JsonParseError::SectionNotFound:
                    return "section not found";
                case JsonParseError::KeyNotFound:
                    return "key not found";
                case JsonParseError::UnknownError:
                    return "unknown error";
                default:
                    return "unknown json parse error";
            }
        };
    };

    inline JsonParseErrorCategory& configurationParseErrorCategory() {
        static JsonParseErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(JsonParseError e) {
        return {static_cast<int>(e), configurationParseErrorCategory()};
    }
}

namespace std {
    template <>
    struct is_error_code_enum<Json::JsonParseError> : std::true_type {
    };
};

namespace Json {
    class JsonHelper {
    public:
        static std::expected<nlohmann::json, std::error_code> loadSection(
            const std::filesystem::path& path, const std::string& section);
        template <typename T>
        static std::expected<T, std::error_code> getValue(const nlohmann::json& object, const std::string& key);
    };

    template <typename T>
    std::expected<T, std::error_code> JsonHelper::getValue(const nlohmann::json& object,
                                                                    const std::string& key) {
        try {
            return object.at(key).get<T>();
        } catch (const nlohmann::json::out_of_range& e) {
            return std::unexpected(JsonParseError::KeyNotFound);
        } catch (const nlohmann::json::type_error& e) {
            return std::unexpected(JsonParseError::InvalidFormat);
        } catch (...) {
            return std::unexpected(JsonParseError::UnknownError);
        }
    }
}

#endif //LIQUIDPETPROJECT_CONFIGURATIONLOADER_H
