add_executable(loggingTest loggingtest.cpp)
target_include_directories(loggingTest PRIVATE  ../../include/logger)
target_link_libraries(loggingTest  liblogger)