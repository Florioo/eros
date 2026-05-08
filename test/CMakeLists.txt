cmake_minimum_required(VERSION 3.24)

set(UNITY_DIR "$ENV{UNITY_DIR}" CACHE PATH "Path to Unity test framework")
if(NOT UNITY_DIR)
    set(UNITY_DIR "/opt/Unity" CACHE PATH "" FORCE)
endif()

if(NOT EXISTS "${UNITY_DIR}/src/unity.c")
    message(FATAL_ERROR
        "Unity not found at ${UNITY_DIR}. "
        "Set UNITY_DIR to a Unity checkout, or rebuild the devcontainer to "
        "populate /opt/Unity.")
endif()

function(eros_add_unity_test name source)
    add_executable(${name} ${source} ${UNITY_DIR}/src/unity.c)
    target_include_directories(${name} PRIVATE ${UNITY_DIR}/src)
    target_link_libraries(${name} PRIVATE eros)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 30)
endfunction()

eros_add_unity_test(primitive_test    primitive_test.c)
eros_add_unity_test(pub_sub_test      pub_sub_test.c)
eros_add_unity_test(server_client_test server_client_test.c)
