#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <algorithm>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(unsigned int size = std::thread::hardware_concurrency()) {
        size = size ? size : 2;

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
        _cv.notify_all();
        std::for_each(_threads.begin(), _threads.end(),
            std::mem_fn(&std::thread::join));
    }

    void Submit(const std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(_work_queue_mutex);
            _work_queue.push(task);
        }

        _cv.notify_one();
    }

private:
    void Worker() {
        while (!_done) {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(_work_queue_mutex);
                if (!_work_queue.empty() && !_done) {
                    task = _work_queue.front();
                    _work_queue.pop();
                }
            }

            if (task) {
                try {
                    task();
                } catch (...) {
                }
            } else if (!_done) {
                std::unique_lock<std::mutex> lock(_work_queue_mutex);
                _cv.wait(lock);
            }
        }
    }

    std::atomic_bool _done{false};
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _work_queue;
    std::mutex _work_queue_mutex;
    std::condition_variable _cv;
};
