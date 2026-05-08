cmake_minimum_required(VERSION 3.15...3.26)

set(EROS_CORE_SRCS
    src/eros_c/eros_endpoint.c
    src/eros_c/eros_package.c
    src/eros_c/eros_router.c
    src/eros_c/eros_worker.c
)
set(EROS_INCLUDE_DIR src/eros_c)

# If ESP-IDF is reading this CMakeLists.txt, register it as an ESP-IDF component
if(DEFINED ENV{IDF_PATH})
    idf_component_register(
        SRCS ${EROS_CORE_SRCS} src/eros_c/port/freertos.c
        INCLUDE_DIRS ${EROS_INCLUDE_DIR}
        REQUIRES freertos
    )
    return()
endif()

# If Zephyr is reading this CMakeLists.txt, register it as a Zephyr module
if(Zephyr_FOUND)
    zephyr_library_named(eros)
    # Zephyr exposes a FreeRTOS-compatible API via CONFIG_FREERTOS_KERNEL; using
    # that here keeps the port surface tiny. A native Zephyr port can be added
    # later under src/eros_c/port/zephyr.c.
    zephyr_library_sources(${EROS_CORE_SRCS} src/eros_c/port/freertos.c)
    target_include_directories(eros PUBLIC ${EROS_INCLUDE_DIR})
    return()
endif()

# Otherwise: standalone host build (Linux / macOS / Windows) using the POSIX
# port. This is what the C tests link against — no FreeRTOS required.
project(eros VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(_COV_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    else()
        set(_COV_FLAGS --coverage)
    endif()
    add_compile_options(${_COV_FLAGS})
    add_link_options(${_COV_FLAGS})
endif()

enable_testing()

add_library(eros STATIC ${EROS_CORE_SRCS} src/eros_c/port/posix.c)
target_include_directories(eros PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${EROS_INCLUDE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(eros PUBLIC Threads::Threads)

add_subdirectory(test)
