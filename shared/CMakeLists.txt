set(SHARED_PUBLIC_HEADERS
    include/logging/Logger.h
    include/json/Json.h
    include/http/HttpMessage.h
)

set(SHARED_SOURCES
    src/logging/Logger.cpp
    src/json/Json.cpp
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

if(BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(order_shared_tests
        src/json/tests/JsonHelperTest.cpp
    )

    target_link_libraries(order_shared_tests PRIVATE
        shared
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_shared_tests)
endif()
