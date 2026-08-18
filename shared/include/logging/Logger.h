//
// Created by DED on 17.08.2026.
//

#ifndef LIQUIDPETPROJECT_LOGGER_H
#define LIQUIDPETPROJECT_LOGGER_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace Logger {
    inline std::vector<spdlog::sink_ptr> sinks;
    inline spdlog::level::level_enum defaultLevel = spdlog::level::info;

    void init(bool enableConsole, bool enableFile, spdlog::level::level_enum level,
              const std::string& fileName, std::size_t maxSizeMB, std::size_t maxFiles);

    std::shared_ptr<spdlog::logger> get(std::string_view category);

    void setLoggingLevel(std::string_view category, spdlog::level::level_enum level);

    void setFileFilter(std::string_view filename);
    void clearFileFilter();
}

#endif //LIQUIDPETPROJECT_LOGGER_H
