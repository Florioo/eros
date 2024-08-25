
cmake_minimum_required(VERSION 3.24)
project(test)


include_directories(Unity/src)

add_executable(pub_sub pub_sub.c)
target_link_libraries(pub_sub PRIVATE eros linux_freertos)


add_executable(primitive_test primitive_test.c Unity/src/unity.c)
target_link_libraries(primitive_test PRIVATE eros linux_freertos)

# Add test  
add_test(NAME primitive_test COMMAND primitive_test)