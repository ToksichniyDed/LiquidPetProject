add_subdirectory(handlers)

add_executable(gateway_service
    main.cpp)

find_package(CLI11 REQUIRED CONFIG)

target_link_libraries(gateway_service PRIVATE gateway_handlers shared_logging shared_json CLI11::CLI11)
