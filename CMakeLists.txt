cmake_minimum_required(VERSION 3.10)

project(eros)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DprojCOVERAGE_TEST=1)
add_definitions(-DprojENABLE_TRACING=0)

# Regular CMake project
add_library(eros STATIC
    src/eros_endpoint.c
    src/eros_package.c
    src/eros_router.c
)
add_subdirectory(freertos_lib)

target_link_libraries(eros linux_freertos)

target_include_directories(eros PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(test)
