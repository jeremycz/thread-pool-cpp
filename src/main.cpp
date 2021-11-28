#include <iostream>

#include "../thread_pool.hpp"

void TestThreadPoolWorker() {
    std::condition_variable cv;
    std::mutex mutex;

    ThreadPoolWorker worker;
    worker.Start();

    worker.SetTask([]() -> void {
        LOG("Task started");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOG("Task finished");
    }, &cv);

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock);

    worker.SetTask([]() -> void {
        LOG("Task started");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOG("Task finished");
    }, &cv);

    cv.wait(lock);

    // stop during task execution
    worker.SetTask([]() -> void {
        LOG("Task started");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        LOG("Task finished");
    }, &cv);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    worker.Stop();
}

void TestThreadPool() {
    // ThreadPool thread_pool;
    // thread_pool.Start();
    // std::this_thread::sleep_for(std::chrono::seconds(3));
}

int main() {
    TestThreadPoolWorker();
    TestThreadPool();
    return 0;
}