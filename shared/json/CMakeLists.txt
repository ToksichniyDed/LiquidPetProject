add_library(shared_json STATIC
    Json.cpp)

find_package(nlohmann_json REQUIRED CONFIG)
target_link_libraries(shared_json PUBLIC shared_include_paths nlohmann_json::nlohmann_json)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(shared_json_tests
        ./tests/JsonHelperTest.cpp
    )

    target_link_libraries(shared_json_tests PRIVATE
        shared_json
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_json_tests)
endif ()
