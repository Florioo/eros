
add_executable(eros_test main.c)

target_link_libraries(eros_test PRIVATE eros linux_freertos)

add_custom_target(run
    COMMAND eros_test
    DEPENDS eros_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


set_target_properties(eros_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
)