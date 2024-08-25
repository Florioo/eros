# Each Makefile lines are reported in this file with "->" 
# and the CMake implementation follows

project(posix_demo C)
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER "gcc")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# For debug, no change detected ...
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS")

include_directories(
    .
    ../src
    ${FREERTOS_DIR}/Source/include
    ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix
    ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
    ${FREERTOS_DIR}/Demo/Common/include
    ${FREERTOS_DIR}/Demo/Posix_GCC

)

FILE(GLOB FREERTOS_SOURCES
    ${FREERTOS_DIR}/Source/*.c
)

# Memory manager (use malloc() / free())
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c")

# Posix port
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c")
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c")

# -> CFLAGS := -ggdb3 -O0 -DprojCOVERAGE_TEST=0 -D_WINDOWS_
add_definitions(-DprojCOVERAGE_TEST=0)
add_definitions(-D_WINDOWS_)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3 -O0 -Wall -Wextra -Werror")

# -> LDFLAGS := -ggdb3 -O0 -pthread
# pthread included after
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ggdb3 -O0")

# Debug
message("Project: ${PROJECT_NAME}")
message("Sources: ${FREERTOS_SOURCES}")

# -> End of Makefile : Compilation

# Create executable with demo code files
add_executable(${PROJECT_NAME}
    ../src/eros.c
    main.c
    hooks.c
)

# Link FreeRTOS as a static library
add_library(freertos STATIC ${FREERTOS_SOURCES})
target_link_libraries(${PROJECT_NAME} freertos)

## Add pthread
target_link_libraries(${PROJECT_NAME} pthread)


# Add make run command
add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)
