//
// Created by DED on 22.08.2026.
//

#include "Json.h"

#include <fstream>

namespace shared::json {
    std::expected<nlohmann::json, std::error_code> JsonHelper::loadSection(const std::filesystem::path& path,
                                                                           const std::string& section) {
        std::ifstream file(path);

        if (!file.is_open()) return std::unexpected(JsonParseError::InvalidPath);

        try {
            const auto data = nlohmann::json::parse(file);
            return data.at(section);
        } catch ([[maybe_unused]] const nlohmann::json::parse_error& e) {
            return std::unexpected(JsonParseError::InvalidFormat);
        } catch ([[maybe_unused]] const nlohmann::json::out_of_range& e) {
            return std::unexpected(JsonParseError::SectionNotFound);
        } catch (...) {
            return std::unexpected(JsonParseError::UnknownError);
        }
    }
}  // namespace shared::json
