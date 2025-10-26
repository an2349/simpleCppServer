//
// Created by an on 10/16/25.
//

#ifndef LTHTM_THREADPOOL_H
#define LTHTM_THREADPOOL_H


#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <nlohmann/detail/exceptions.hpp>

class ThreadPool {
public:
    explicit ThreadPool(size_t n);

    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F &&f, Args &&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        try {
            using return_type = typename std::invoke_result<F, Args...>::type;
            auto task = std::make_shared<std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace([task]() { (*task)(); });
            }
            condition.notify_one();
            return res;
        }
        catch (std::exception &e) {
            std::cout<<e.what();;
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()> > tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


#endif //LTHTM_THREADPOOL_H
