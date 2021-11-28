#include <iostream>
#include <mutex>

#include "../thread_pool.hpp"

void TestThreadPool() {
    ThreadPool thread_pool(5);

    std::mutex cout_mutex;

    for (int i = 0; i < 20; ++i) {
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << std::this_thread::get_id() << " | Submitting Task " << i << std::endl;
        }
        
        thread_pool.Submit([i, &cout_mutex]() {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << std::this_thread::get_id() << " | Starting Task " << i << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << std::this_thread::get_id() << " | Finishing Task " << i << std::endl;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
}

int main() {
    TestThreadPool();
    return 0;
}