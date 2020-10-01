//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include "ThreadPool.h"
#include "Epoll.h"

class Server {
public:
    Server(size_t threadNum, int port);

    ~Server();

    [[noreturn]] void start();

private:
    std::unique_ptr<ThreadPool> _threadPool;
    std::shared_ptr<Epoll> _poller;
    int _listenFd;
};