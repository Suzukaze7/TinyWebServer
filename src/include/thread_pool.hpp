#pragma once
#include <concepts>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace suzukaze {
template <typename T>
concept Task = std::invocable<T> && std::movable<T>;

template <Task T = std::function<void()>>
class ThreadPool {
    const std::size_t THREAD_COUNT_;
    std::atomic_bool done_;
    std::queue<T, std::list<T>> que_;
    std::mutex lock_;
    std::condition_variable cv_;
    std::vector<std::jthread> threads_;

    void work() {
        while (!done_) {
            std::unique_lock guard(lock_);
            cv_.wait(guard, [&] { return !que_.empty() || done_; });

            if (done_)
                break;

            auto task = std::move(que_.front());
            que_.pop();
            guard.unlock();
            task();
        }
    }

public:
    ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency())
        : THREAD_COUNT_(thread_count) {
        for (std::size_t i = 0; i < THREAD_COUNT_; i++)
            threads_.emplace_back(&ThreadPool::work, this);
    }

    ThreadPool(const ThreadPool &) = delete;

    ~ThreadPool() {
        done_ = true;
        cv_.notify_all();
    }

    void submit(T task) {
        std::unique_lock guard(lock_);
        que_.push(std::move(task));
        guard.unlock();
        cv_.notify_one();
    }
};
} // namespace suzukaze