//
// Created by Yvan on 2020/9/23.
//

#include "Server.h"
#include "Utils.h"
#include "HttpData.h"

Server::Server(size_t threadNum, int port)
        : _threadPool(new ThreadPool(threadNum)),
          _poller(new Epoll()),
          _listenFd(socket_bind_listen(port)) {
    handle_for_sigpipe();
    if (setSocketNonBlocking(_listenFd) < 0) {
        perror("set socket non block failed");
        abort();
    }
}


Server::~Server() = default;

[[noreturn]] void Server::start() {
    uint32_t event = EPOLLIN | EPOLLET;
    std::shared_ptr<HttpData> http_data = std::make_shared<HttpData>(_listenFd, _poller);
    _poller->epoll_add(_listenFd, event, http_data);
    while (true) {
        auto requests = _poller->poll(_listenFd);
        // 将事件传递给 线程池
        for (const auto &req : requests) {
            _threadPool->submit([this, req]() {
                req->handleRequest();
            });
        }
        // 处理定时器超时事件
        _poller->handleExpired();
    }
}

