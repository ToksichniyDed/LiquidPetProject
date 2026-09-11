add_library(worker_processing STATIC
OrderReservationProcessor.cpp)

target_include_directories(worker_processing INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(worker_processing PUBLIC
IOrderProcessor.h)

target_link_libraries(worker_processing PUBLIC
worker_events
PRIVATE
shared_logging)
