//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include <memory>
#include <queue>

class HttpData;

class TimerNode {
public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);

    ~TimerNode();

    TimerNode(TimerNode &tn);

    bool isValid();

    void clearRequest();

    void setDeleted();

    bool isDeleted() const;

    size_t getExpTime() const;
    void update(int timeout);
private:
    bool _deleted{};
    size_t _expiredTime{};
    std::shared_ptr<HttpData> spHttpData;
};

struct TimerCmp {
    bool operator()(std::shared_ptr<TimerNode> &a,
                    std::shared_ptr<TimerNode> &b) const {
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerManager{
public:
    TimerManager();
    ~TimerManager();
    void addTimer(const std::shared_ptr<HttpData>&spHttpData, int timeout);
    void handleExpiredEvent();

private:
    using TimerNodeSPtr = std::shared_ptr<TimerNode>;
    std::priority_queue<TimerNodeSPtr, std::deque<TimerNodeSPtr>, TimerCmp> timerNodeQueue;
};