cmake_minimum_required(VERSION 3.10)
project(webserver)

set(CMAKE_CXX_STANDARD 11)
add_executable(webserver main.cpp)
target_include_directories(webserver PRIVATE  include/logger include/webserver)
target_link_libraries(webserver liblogger libserver)
add_subdirectory(src)
add_custom_command(TARGET webserver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/files/index.html $<TARGET_FILE_DIR:webserver>)