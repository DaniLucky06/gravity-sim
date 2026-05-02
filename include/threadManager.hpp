#include <mutex>
#include <queue>
#include <thread>
#include <future>
#include <condition_variable>

#include "grid.hpp"

class ThreadPool {
private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

public:
    ThreadPool(int numThreads = std::thread::hardware_concurrency()) 
    {
        for (size_t i = 0; i < numThreads; i++) {
            threads.emplace_back([&] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [&] {
                            return !tasks.empty() || stop;
                        });

                        if (tasks.empty() || stop) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
        
    };

    ~ThreadPool() 
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
        }

        cv.notify_all();

        // CHECK FOR FUNCTIONALITY
        for (std::thread& thread : threads) {
            thread.join();
        }
    };

    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) 
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.emplace([task = std::forward<F>(task), args] 
            {
                task(args);
            });
        }

        cv.notify_one();
    };
};