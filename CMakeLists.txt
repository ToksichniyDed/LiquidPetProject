cmake_minimum_required(VERSION 4.2.3)
project(LiquidPetProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(shared)
add_subdirectory(order-service)
# add_subdirectory(gateway)
