add_subdirectory(postgres)

add_library(shared_repository INTERFACE)

target_include_directories(shared_repository INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(shared_repository INTERFACE
    RepositoryError.h)
