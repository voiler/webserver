//
// Created by Yvan on 2020/9/23.
//

#include "Epoll.h"
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Logging.h"
#include <cstring>
#include "Utils.h"
#include <unistd.h>

const int EVENTSNUM = 4096; // 监听的最大事件
const int EPOLLWAIT_TIME = 500;// 计时器最大等待时间 (ms)


Epoll::Epoll() : _epollFd(epoll_create1(EPOLL_CLOEXEC)), _events(EVENTSNUM) {
    assert(_epollFd > 0);
}

Epoll::~Epoll() = default;

// 注册新描述符
void Epoll::epoll_add(int fd, uint32_t events, std::shared_ptr<HttpData> httpData) {
//    if (timeout > 0) {
//        add_timer(request, timeout);
//        _fd2http[fd] = request->getHolder();
//    }
    struct epoll_event event{};
    event.data.fd = fd;
    event.events = events;
    _fd2http[fd] = std::move(httpData);
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_add error");
        _fd2http[fd]->reset();
    }
}

// 修改描述符状态
void Epoll::epoll_mod(int fd, uint32_t events, std::shared_ptr<HttpData> httpData) {
//    if (timeout > 0) add_timer(request, timeout);
    struct epoll_event event{};
    event.data.fd = fd;
    event.events = events;

    if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event) < 0) {
        perror("epoll_mod error");
        _fd2http[fd]->reset();
    }
    _fd2http[fd] = std::move(httpData);
}

// 从epoll中删除描述符
void Epoll::epoll_del(int fd, uint32_t events) {
    struct epoll_event event{};
    event.data.fd = fd;
    event.events = events;
    // event.events = 0;
    // request->EqualAndUpdateLastEvents()
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("epoll_del error");
    }
    _fd2http[fd]->reset();
}

// 返回活跃事件数
std::vector<HttpDataPtr> Epoll::poll(const int &listenFd) {
    int event_count =
            epoll_wait(_epollFd, &*_events.begin(), _events.size(), EPOLLWAIT_TIME);
    if (event_count < 0) {
        perror("epoll wait error");
    }
    std::vector<HttpDataPtr> req_data = getEventsRequest(listenFd, event_count);
    if (!req_data.empty()) {
        return req_data;
    }
    return {};
}

void Epoll::handleExpired() {
    _timerManager.handleExpiredEvent();
}

// 分发处理函数
std::vector<HttpDataPtr> Epoll::getEventsRequest(const int &listenFd, int events_num) {
    std::vector<HttpDataPtr> req_data;
    for (int i = 0; i < events_num; ++i) {
        // 获取有事件产生的描述符
        int fd = _events[i].data.fd;
        if (fd < 3) {
            LOG << "fd is less than 3";
            continue;
        } else if (fd == listenFd) {
            acceptConnection(listenFd);
        } else {
            uint32_t events = _events[i].events;
            if (((events & EPOLLHUP) && !(events & EPOLLIN)) || events & EPOLLERR) {
                _fd2http[fd]->reset();
                continue;
            }
            HttpDataPtr cur_req = _fd2http[fd];
            cur_req->timerDetach();
            req_data.emplace_back(cur_req);
            _fd2http[fd]->reset();
        }
    }
    return req_data;
}

void Epoll::acceptConnection(int listenFd) {
    struct sockaddr_in client_addr{};
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = 0;
    while ((client_fd = accept(listenFd, (struct sockaddr *) &client_addr, &client_addr_len)) > 0) {
        if (client_fd < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                return;
            LOG << "client_fd:" << client_fd << " accept error in file <" << __FILE__ << "> " << "at " << __LINE__;
            perror("accept error");
            //exit(0);
        }
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
            << ntohs(client_addr.sin_port);
        if (client_fd >= MAXFDS) {
            close(client_fd);
            continue;
        }
        // 设为非阻塞模式
        if (setSocketNonBlocking(client_fd) < 0) {
            LOG << "Set non block failed!";
            // perror("Set non block failed!");
            return;
        }
        setSocketNodelay(client_fd);
        auto data = std::make_shared<HttpData>(client_fd,shared_from_this());
        _timerManager.addTimer(data, EPOLLWAIT_TIME);
        epoll_add(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT, data);  //保证同一SOCKET只能被一个线程处理
    }

}
