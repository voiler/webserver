//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include "NonCopyAble.h"
#include "LogStream.h"
#include <vector>
#include <thread>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
class AsyncLogging : NonCopyAble {
public:
    explicit AsyncLogging(const std::string &basename, int flushInterval = 2);

    ~AsyncLogging();

    void append(const char *logline, int len);

    void stop();

private:
    void threadFunc();

    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::shared_ptr<Buffer>>;
    using BufferPtr = std::shared_ptr<Buffer>;

    const int _flushInterval;
    bool _running;
    std::string _basename{};
    std::thread _thread{};
    std::mutex _mutex;
    std::condition_variable _condition;
    BufferPtr _currentBuffer;
    BufferPtr _nextBuffer;
    BufferVector _buffers;
};