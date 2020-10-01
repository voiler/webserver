//
// Created by Yvan on 2020/9/28.
//

#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    inline explicit ThreadPool(size_t threads)
            : _shutdown(false) {
        for (size_t i = 0; i < threads; ++i)
            _workers.emplace_back(
                    [this] {
                        while (true) {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(this->_queue_mutex);
                                this->_condition.wait(lock,
                                                      [this] { return this->_shutdown || !this->_tasks.empty(); });
                                if (this->_shutdown && this->_tasks.empty())
                                    return;
                                task = std::move(this->_tasks.front());
                                this->_tasks.pop();
                            }

                            task();
                        }
                    }
            );
    }

    bool submit(std::function<void()> &&func) {
        {
            std::unique_lock<std::mutex> lock(_queue_mutex);

            // don't allow enqueueing after stopping the pool
            if (_shutdown)
                throw std::runtime_error("try to submit a task to stopped ThreadPool");

            _tasks.emplace(func);
        }
        _condition.notify_one();
        return true;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _shutdown = true;
        }
        _condition.notify_all();
        for (std::thread &worker: _workers)
            worker.join();
    }

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> _workers;
    // the task queue
    std::queue<std::function<void()> > _tasks;

    // synchronization
    std::mutex _queue_mutex;
    std::condition_variable _condition;
    bool _shutdown;
};


