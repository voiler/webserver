#include <iostream>
#include <getopt.h>
#include "Logging.h"
#include "Server.h"

int main(int argc, char *argv[]) {
    int opt, threadNum = 4, port = 8080;
    std::string logPath = "./web_server.log";
    const char *str = "t:l:p";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 't':
                threadNum = atoi(optarg);
                break;
            case 'l':
                logPath = optarg;
                if (logPath.size() < 2 || optarg[0] != '/') {
                    printf("logPath should start with \"/\"\n");
                }
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                break;
        }
    }
    Logger::setLogFileName(logPath);
    Server httpServer(threadNum, port);
    httpServer.start();
    return 0;
}
