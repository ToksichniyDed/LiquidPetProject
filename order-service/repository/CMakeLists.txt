add_library(order_repository INTERFACE)

set(ORDER_REPOSITORY_HEADERS
    IOrderRepository.h
)

target_include_directories(order_repository INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(order_repository INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${ORDER_REPOSITORY_HEADERS}
)

target_compile_features(order_repository INTERFACE cxx_std_23)
target_link_libraries(order_repository INTERFACE order_models)

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
