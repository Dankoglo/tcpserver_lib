#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <queue>
#include <vector>

#include <functional>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#include <iostream>


/*
    Simple thread pool.
    
    Creates and ownes a particular number of threads.
    Assigns tasks to the owned threads.
*/

class ThreadPool {
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> threads;

    std::mutex mtx;
    std::condition_variable cv;
    bool stopping = false;

    int busy_workers;
public:
    ThreadPool() = default;

    ThreadPool(ThreadPool&) = delete;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool();

    void start(int);
    void stop();

    int busy_threads()
    {
        std::unique_lock<std::mutex> lock(mtx);
        return busy_workers;
    }
    
    template<typename T>
    auto execute_task(T task)
    {
        std::shared_ptr <std::packaged_task<decltype(task()) ()>> wrapper =
		    std::make_shared<std::packaged_task<decltype(task()) ()>>(std::move(task));

		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.emplace([=] { (*wrapper) (); });
		}

		cv.notify_one();
		return wrapper->get_future();
    }
};

#endif // THREAD_POOL_HPP