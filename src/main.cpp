#include <iostream>
#include <mutex>

#include "../thread_pool.hpp"

void TestThreadPool(const int64_t task_duration) {
    std::cout << "=== TestThreadPool(" << task_duration << ") ===" << std::endl;
    ThreadPool thread_pool(5);

    std::mutex cout_mutex;

    for (int i = 0; i < 20; ++i) {
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << std::this_thread::get_id() << " | Submitting Task " << i << std::endl;
        }
        
        thread_pool.Submit([i, task_duration, &cout_mutex]() {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << std::this_thread::get_id() << " | Starting Task " << i << std::endl;
            }
            
            if (task_duration > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(task_duration));
            }
            
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << std::this_thread::get_id() << " | Finishing Task " << i << std::endl;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
}

int main() {
    TestThreadPool(0);
    TestThreadPool(1);
    return 0;
}