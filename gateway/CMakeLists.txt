add_subdirectory(handlers)

add_executable(gateway_service
    main.cpp)

target_link_libraries(gateway_service PRIVATE gateway_handlers shared_logging shared_json)
