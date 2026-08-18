set(SHARED_PUBLIC_HEADERS
    include/logging/Logger.h
)

set(SHARED_SOURCES
    src/logging/Logger.cpp
)

add_library(shared STATIC ${SHARED_SOURCES})

target_sources(shared PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES ${SHARED_PUBLIC_HEADERS}
)

find_package(spdlog REQUIRED CONFIG)
target_link_libraries(shared PUBLIC spdlog::spdlog)