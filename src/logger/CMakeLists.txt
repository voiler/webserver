set(LOGGER_SRC
        FileUtil.cpp
        LogFile.cpp
        AsyncLogging.cpp
        LogStream.cpp
        Logging.cpp
        )

add_library(liblogger ${LOGGER_SRC})

target_include_directories(liblogger PRIVATE  ../../include/logger)
target_link_libraries(liblogger pthread rt)
set_target_properties(liblogger PROPERTIES OUTPUT_NAME "logger")