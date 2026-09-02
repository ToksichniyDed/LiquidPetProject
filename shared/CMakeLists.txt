add_library(shared_include_paths INTERFACE)
target_include_directories(shared_include_paths INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(logging)
add_subdirectory(json)
add_subdirectory(http)
