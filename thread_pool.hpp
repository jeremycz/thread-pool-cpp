#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>

class ThreadPool {
public:
    ThreadPool(unsigned int size = std::thread::hardware_concurrency()) {
        size = size ? size : std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < size; ++i) {
                _threads.push_back(std::thread(&ThreadPool::Worker, this));
            }
        } catch (...) {
            _done = true;
            throw;
        }
    }

    ~ThreadPool() {
        _done = true;
        std::for_each(_threads.begin(), _threads.end(),
            std::mem_fn(&std::thread::join));
    }

    void Submit(const std::function<void()> task) {
        std::lock_guard<std::mutex> lock(_work_queue_mutex);
        _work_queue.push(task);
    }

private:
    void Worker() {
        while (!_done) {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(_work_queue_mutex);
                if (!_work_queue.empty()) {
                    task = _work_queue.front();
                    _work_queue.pop();
                }
            }

            if (task) {
                try {
                    task();
                } catch (...) {
                }
            } else {
                std::this_thread::yield();
            }
        }
    }

    std::atomic_bool _done{false};
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _work_queue;
    std::mutex _work_queue_mutex;
};
