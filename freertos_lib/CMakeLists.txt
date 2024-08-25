cmake_minimum_required(VERSION 3.5)
project(freertos_linux_project)

# Set the FreeRTOS source directory
set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS")

# Gather FreeRTOS source files
FILE(GLOB FREERTOS_SOURCES
    ${FREERTOS_DIR}/Source/*.c
)
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c")
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c")
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c")
list(APPEND FREERTOS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hooks.c")


# Create the FreeRTOS static library
add_library(linux_freertos STATIC ${FREERTOS_SOURCES})

# Export the library for use in other projects
target_include_directories(linux_freertos PUBLIC
    ${FREERTOS_DIR}/Source/include
    ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix
    ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
    ${FREERTOS_DIR}/Demo/Common/include
    ${FREERTOS_DIR}/Demo/Posix_GCC
    .
)

target_link_libraries(linux_freertos pthread)
# target_link_libraries(linux_freertos freertos)
