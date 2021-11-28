#pragma once

#include <functional>
#include <thread>
#include <queue>
#include <iostream>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <sstream>
#include <ctime>

/*
1. Allocate task to a thread
2. Wake thread
3. Run task
4. Wake allocator thread
5. Find next free thread
6. Return to 1.
*/

void log(const std::string& text) {
    auto time_now = std::chrono::system_clock::now();
    int64_t ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch()).count() % 1000;
    auto time_t_now = std::chrono::system_clock::to_time_t(time_now);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
    std::cout << time_str << "." << ms_count << " : " << text << std::endl;
}

#define LOG(s) { std::stringstream ss; ss << s; log(ss.str()); }

class ThreadPoolWorker {
public:
    ThreadPoolWorker() {}

    virtual ~ThreadPoolWorker() {
        if (_running) {
            Stop();
        }
    }

    void Start() {
        _running = true;
        _thread = std::thread(std::bind(&ThreadPoolWorker::MainThread, this));

        // ensure MainThread has entered loop and is waiting
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        _cv_ready.wait(lock);
    }

    void Stop() {
        LOG("ThreadPoolWorker: Stopping");
        _running = false;
        if (_thread.joinable()) {
            _cv_task.notify_one();
            _thread.join();
        }
    }

    bool SetTask(const std::function<void()> task, std::condition_variable* cv_complete) {
        if (_busy) {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _busy = true;
            _task = task;
            _cv_complete = cv_complete;
        }
        
        LOG("ThreadPoolWorker: New task");
        _cv_task.notify_one();
        
        return true;
    }

    bool Busy() const {
        return _busy.load();
    }
    
private:
    void MainThread() {
        while (_running) {
            try {
                LOG("ThreadPoolWorker: Waiting");
                std::unique_lock<std::mutex> lock(_mutex);
                if (_cv_complete) {
                    _cv_complete->notify_one();
                }
                _cv_ready.notify_one();
                _cv_task.wait(lock);
                                
                if (!_running) {
                    break;
                } else if (_task) {
                    LOG("ThreadPoolWorker: Running task");
                    _task();
                }
            } catch(std::exception exception) {
                LOG("ThreadPoolWorker: Failed to run task - " << exception.what());
            }

            _task = nullptr;
            _busy = false;
        }
    }

    std::thread _thread;
    std::function<void()> _task = nullptr;
    std::atomic_bool _running = false;
    std::atomic_bool _busy = false;
    std::condition_variable _cv_task;
    std::condition_variable _cv_ready;
    std::condition_variable* _cv_complete;
    std::mutex _mutex;
};

class ThreadPool {
public:
    ThreadPool(const size_t num_workers = 10) :
        _next_worker(_workers.begin()),
        _num_workers(num_workers) {
        // TODO: check num_workers > 0

        for (size_t i = 0; i < _num_workers; ++i) {
            _workers.push_back(std::make_unique<ThreadPoolWorker>());
        }

        for (auto& worker : _workers) {
            worker->Start();
        }
    }

    virtual ~ThreadPool() {
        Stop();
    }

    void Start() {
        _running = true;
        _main_thread = std::thread(std::bind(&ThreadPool::MainThread, this));

        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        _cv_ready.wait(lock);
    }

    void Stop() {
        LOG("ThreadPool stopping");
        _running = false;
        if (_main_thread.joinable()) {
            _main_thread.join();
        }

        for (auto& worker : _workers) {
            worker->Stop();
        }
    }

    void SubmitTask(const std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tasks.push(task);
        }

        _cv.notify_one();
    }

private:
    void MainThread() {
        size_t count = 0;
        while (_running) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv_ready.notify_one();
                _cv.wait(lock);
                
                if (!_running) {
                    break;
                } else if (!_tasks.empty()) {
                    // find next available worker
                    size_t count = 0;
                    while (count < _num_workers) {
                        if (!(*_next_worker)->Busy()) {
                            break;
                        }

                        ++_next_worker;
                        ++count;
                        if (_next_worker == _workers.end()) {
                            _next_worker = _workers.begin();
                        }
                    }

                    auto next_task = _tasks.front();
                    _tasks.pop();

                    (*_next_worker)->SetTask(next_task, &_cv);
                    ++_next_worker;
                    if (_next_worker == _workers.end()) {
                        _next_worker = _workers.begin();
                    }
                }
            }
        }
    }

    std::queue<std::function<void()>> _tasks;
    std::thread _main_thread;
    std::atomic_bool _running = false;
    std::vector<std::unique_ptr<ThreadPoolWorker>> _workers;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::condition_variable _cv_ready;
    std::vector<std::unique_ptr<ThreadPoolWorker>>::iterator _next_worker;
    size_t _num_workers;
};
