#pragma once
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>

namespace suzukaze {
template<typename T>
concept Task = std::invocable<T> && std::copyable<T>;

template <Task T = std::function<void()>>
class ThreadPool {
public:
    ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency(),
               std::size_t max_queue_size = 1000);
    ThreadPool(const ThreadPool &) = delete;
    ~ThreadPool();

private:
    const std::size_t THREAD_COUNT;
    const std::size_t MAX_QUEUE_SIZE;
    std::atomic_bool done;
    std::queue<T> q;
    std::vector<std::jthread> threads;
    std::mutex lock;
    std::condition_variable consume_cv, support_cv;

    void work();

public:
    void submit(T task);
};

template <Task T>
inline ThreadPool<T>::ThreadPool(std::size_t thread_count, std::size_t max_queue_size)
    : THREAD_COUNT(thread_count), MAX_QUEUE_SIZE(max_queue_size) {
    for (std::size_t i = 0; i < THREAD_COUNT; i++)
        threads.emplace_back(&ThreadPool::work, this);
}

template <Task T>
inline ThreadPool<T>::~ThreadPool() {
    done = true;
    consume_cv.notify_all();
}

template <Task T>
inline void ThreadPool<T>::work() {
    while (!done) {
        std::unique_lock guard(lock);
        consume_cv.wait(guard, [&] { return !q.empty() || done; });

        if (done)
            break;

        auto task = std::move(q.front());
        q.pop();
        guard.unlock();
        support_cv.notify_one();
        task();
    }
}

template <Task T>
inline void ThreadPool<T>::submit(T task) {
    std::unique_lock guard(lock);
    support_cv.wait(guard, [&] { return q.size() < MAX_QUEUE_SIZE; });
    q.push(std::move(task));
    consume_cv.notify_one();
}
} // namespace suzukaze