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
    const std::size_t THREAD_COUNT_;
    const std::size_t MAX_QUEUE_SIZE_;
    std::atomic_bool done_;
    std::queue<T> que_;
    std::vector<std::jthread> threads_;
    std::mutex lock_;
    std::condition_variable consume_cv_, support_cv_;

    void work();

public:
    void submit(T task);
};

template <Task T>
inline ThreadPool<T>::ThreadPool(std::size_t thread_count, std::size_t max_queue_size)
    : THREAD_COUNT_(thread_count), MAX_QUEUE_SIZE_(max_queue_size) {
    for (std::size_t i = 0; i < THREAD_COUNT_; i++)
        threads_.emplace_back(&ThreadPool::work, this);
}

template <Task T>
inline ThreadPool<T>::~ThreadPool() {
    done_ = true;
    consume_cv_.notify_all();
}

template <Task T>
inline void ThreadPool<T>::work() {
    while (!done_) {
        std::unique_lock guard(lock_);
        consume_cv_.wait(guard, [&] { return !que_.empty() || done_; });

        if (done_)
            break;

        auto task = std::move(que_.front());
        que_.pop();
        guard.unlock();
        support_cv_.notify_one();
        task();
    }
}

template <Task T>
inline void ThreadPool<T>::submit(T task) {
    std::unique_lock guard(lock_);
    support_cv_.wait(guard, [&] { return que_.size() < MAX_QUEUE_SIZE_; });
    que_.push(std::move(task));
    consume_cv_.notify_one();
}
} // namespace suzukaze