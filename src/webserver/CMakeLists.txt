set(SERVER_SRC
        Epoll.cpp
        HttpData.cpp
        Server.cpp
        Timer.cpp
        Utils.cpp
        )
add_library(libserver ${SERVER_SRC})
target_include_directories(libserver PRIVATE  ../../include/logger ../../include/webserver)
target_link_libraries(libserver liblogger)
set_target_properties(libserver PROPERTIES OUTPUT_NAME "server")