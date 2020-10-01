//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include <vector>
#include <sys/epoll.h>
#include "HttpData.h"
#include "Timer.h"


class Epoll:  public std::enable_shared_from_this<Epoll> {

public:
    Epoll();

    ~Epoll();

    void epoll_add(int fd, uint32_t events, std::shared_ptr<HttpData> httpData);

    void epoll_mod(int fd, uint32_t events, std::shared_ptr<HttpData> httpData);

    void epoll_del(int fd, uint32_t events);

    std::vector<HttpDataPtr> poll(const int &listenFd);

    std::vector<HttpDataPtr> getEventsRequest(const int &listenFd, int events_num);

    int getEpollFd() const { return _epollFd; }

    void handleExpired();

private:
    void acceptConnection(int listenFd);
    static const int MAXFDS = 100000;
    int _epollFd;
    std::vector<epoll_event> _events;
    std::shared_ptr<HttpData> _fd2http[MAXFDS];
    TimerManager _timerManager;
};