find_package(libpqxx REQUIRED CONFIG)

add_library(order_repository STATIC
    PostgresOrderRepository.cpp
)

target_sources(order_repository
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
    IOrderRepository.h
    PostgresOrderRepository.h
)

target_compile_features(order_repository PUBLIC cxx_std_23)

target_link_libraries(order_repository
    PUBLIC
    order_models
    PRIVATE
    libpqxx::pqxx
)

if(BUILD_TESTING)
    find_package(GTest REQUIRED CONFIG)

    add_library(order_repository_mocks INTERFACE)
    target_sources(order_repository_mocks INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES MockOrderRepository.h
    )
    target_link_libraries(order_repository_mocks INTERFACE
        order_repository
        GTest::gmock
    )

    set(ORDER_REPOSITORY_TEST_SOURCES
        tests/OrderRepositoryTest.cpp
    )

    add_executable(order_repository_tests ${ORDER_REPOSITORY_TEST_SOURCES})
    target_link_libraries(order_repository_tests PRIVATE
        order_repository_mocks
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(order_repository_tests)
endif()
