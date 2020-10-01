//
// Created by Yvan on 2020/9/23.
//
#include "AsyncLogging.h"
#include "LogFile.h"
#include <cassert>

AsyncLogging::AsyncLogging(const std::string &logFileName, int flushInterval) :
        _flushInterval(flushInterval),
        _running(true),
        _basename(logFileName),
        _currentBuffer(new Buffer),
        _nextBuffer(new Buffer),
        _buffers() {
    assert(logFileName.size() > 1);
    _currentBuffer->bzero();
    _nextBuffer->bzero();
    _buffers.reserve(16);
    _thread = std::thread(&AsyncLogging::threadFunc, this);
}

void AsyncLogging::stop() {
    _running = false;
    _condition.notify_one();
    _thread.join();
}

AsyncLogging::~AsyncLogging() {
    if (_running) stop();
}

void AsyncLogging::append(const char *logline, int len) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_currentBuffer->avail() > len)
        _currentBuffer->append(logline, len);
    else {
        _buffers.push_back(_currentBuffer);
        _currentBuffer.reset();
        if (_nextBuffer)
            _currentBuffer = std::move(_nextBuffer);
        else
            _currentBuffer.reset(new Buffer);
        _currentBuffer->append(logline, len);
        _condition.notify_one();
    }
}

void AsyncLogging::threadFunc() {
    assert(_running);
    LogFile output(_basename);
    BufferPtr newBuffer1 = std::make_shared<Buffer>();
    BufferPtr newBuffer2 = std::make_shared<Buffer>();
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (_running) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_buffers.empty())  // unusual usage!
            {
                _condition.wait_for(lock,std::chrono::seconds(_flushInterval));
            }
            _buffers.emplace_back(std::move(_currentBuffer));
            _currentBuffer.reset();

            _currentBuffer = std::move(newBuffer1);
            buffersToWrite.swap(_buffers);
            if (!_nextBuffer) {
                _nextBuffer = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());
        if (buffersToWrite.size() > 25) {
            // char buf[256];
            // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
            // buffers\n",
            //          Timestamp::now().toFormattedString().c_str(),
            //          buffersToWrite.size()-2);
            // fputs(buf, stderr);
            // output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }
        for (auto &b : buffersToWrite) {
            output.append(b->data(), b->length());
        }
        if (buffersToWrite.size() > 2) {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }
        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
