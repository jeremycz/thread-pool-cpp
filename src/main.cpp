#include <iostream>
#include <chrono>

#include "../thread_pool.hpp"

void TestThreadPool() {
    ThreadPool thread_pool(5);
    for (int i = 0; i < 20; ++i) {
        std::cout << "Submitting Task " << i << std::endl;
        thread_pool.Submit([i]() {
            std::cout << "Starting Task " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Finishing Task " << i << std::endl;
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
}

int main() {
    TestThreadPool();
    return 0;
}