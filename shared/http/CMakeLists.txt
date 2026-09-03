add_library(shared_http STATIC
    HttpMessageConverter.cpp
    HttpServer.cpp)

target_sources(shared_http PUBLIC
    NetworkAddress.h
    NetworkConfiguration.h
    NetworkConfigurationJsonMapper.h
    NetworkConfigurationJsonKeys.h
    Route.h
)

find_package(Boost REQUIRED CONFIG)

target_link_libraries(shared_http PUBLIC shared_include_paths PRIVATE shared_logging Boost::headers)

if (BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_executable(shared_http_models2json_mapper_tests
        tests/NetworkConfigurationJsonMapperTest.cpp
        tests/HttpMessageConverterTest.cpp
        tests/HttpServerTest.cpp
    )

    target_link_libraries(shared_http_models2json_mapper_tests
        PRIVATE
        shared_http
        shared_json
        shared_logging
        Boost::headers
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(shared_http_models2json_mapper_tests)
endif ()
