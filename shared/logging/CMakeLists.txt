add_library(shared_logging STATIC
    Logger.cpp)

find_package(spdlog REQUIRED CONFIG)
target_link_libraries(shared_logging PUBLIC shared_include_paths spdlog::spdlog)


