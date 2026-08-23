//
// Created by DED on 22.08.2026.
//

#include <fstream>

#include "loader/ConfigurationLoader.h"

std::expected<nlohmann::json, std::error_code> Loader::ConfigurationLoader::loadSection(const std::filesystem::path& path,
    const std::string& section) {

    std::ifstream file (path);

    if (!file.is_open())
        return std::unexpected(ConfigurationParseError::InvalidPath);

    try {
        const auto data = nlohmann::json::parse(file);
        return data.at(section);
    }
    catch ([[maybe_unused]] const nlohmann::json::parse_error& e) {
            return std::unexpected(ConfigurationParseError::InvalidFormat);
    }
    catch ([[maybe_unused]] const nlohmann::json::out_of_range& e) {
        return std::unexpected(ConfigurationParseError::SectionNotFound);
    }
    catch (...) {
        return std::unexpected(ConfigurationParseError::UnknownError);
    }
}
