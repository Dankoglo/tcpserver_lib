#include "thread_pool.hpp"

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::start(int number_of_threads)
{
    for(int i = 0; i < number_of_threads; i++)
        threads.emplace_back(
            [=] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this] { return  stopping || !tasks.empty(); });

                        if(stopping && tasks.empty()) return;

                        task = tasks.front();
                        tasks.pop();
                    }

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        busy_workers++;
                    }

                    task();

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        busy_workers--;
                    }
                }
            }
        );
}
 
void ThreadPool::stop()
{

    {
        std::unique_lock<std::mutex> lock(mtx);
        stopping = true;
    }

    cv.notify_all();

    for(int i = 0; i < threads.size(); i++)
        threads[i].join();
}