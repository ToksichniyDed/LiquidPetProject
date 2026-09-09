find_package(Boost REQUIRED CONFIG)

add_library(shared_models INTERFACE)

target_include_directories(shared_models INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(shared_models INTERFACE cxx_std_23)

target_link_libraries(shared_models INTERFACE
    Boost::headers)

if(BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)
    find_package(nlohmann_json REQUIRED CONFIG)

    add_executable(shared_models_tests
        tests/DatabaseConfigurationTest.cpp
    )

    target_link_libraries(shared_models_tests PRIVATE
        shared_models
        nlohmann_json::nlohmann_json
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_models_tests)
endif()
