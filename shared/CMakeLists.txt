set(SHARED_PUBLIC_HEADERS
    include/logging/Logger.h
    include/loader/ConfigurationLoader.h
)

set(SHARED_SOURCES
    src/logging/Logger.cpp
    src/loader/ConfigurationLoader.cpp
)

add_library(shared STATIC ${SHARED_SOURCES})

target_sources(shared PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES ${SHARED_PUBLIC_HEADERS}
)

find_package(spdlog REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
target_link_libraries(shared PUBLIC spdlog::spdlog nlohmann_json::nlohmann_json)
