#include "logging/Logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/null_sink.h>

void Logger::init(bool enableConsole, bool enableFile, spdlog::level::level_enum level,
                  const std::string& fileName, std::size_t maxSizeMB, std::size_t maxFiles) {
    std::vector<spdlog::sink_ptr> loggerSinks;

    if (enableConsole) {
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_level(spdlog::level::trace);
        console->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] [%!] %v");
        loggerSinks.push_back(console);
    }

    if (enableFile) {
        auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            fileName, 1024 * 1024 * maxSizeMB, maxFiles
        );
        file->set_level(spdlog::level::trace);
        file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] [%!] %v");
        loggerSinks.push_back(file);
    }

    if (loggerSinks.empty()) {
        loggerSinks.push_back(std::make_shared<spdlog::sinks::null_sink_st>());
    }

    spdlog::init_thread_pool(8192, 1);

    sinks = std::move(loggerSinks);
    defaultLevel = level;

    get("Logger")->info("Логирование инициализировано (console: {}, file: {})", enableConsole, enableFile);
}

std::shared_ptr<spdlog::logger> Logger::get(std::string_view category) {
    if (auto existing = spdlog::get(std::string(category)))
        return existing;

    auto newLogger = std::make_shared<spdlog::async_logger>(
        std::string(category), sinks.begin(), sinks.end(),
        spdlog::thread_pool(), spdlog::async_overflow_policy::block
    );
    newLogger->set_level(defaultLevel);
    spdlog::register_logger(newLogger);
    return newLogger;
}

void Logger::setLoggingLevel(std::string_view category, spdlog::level::level_enum level) {
    get(category)->set_level(level);
}