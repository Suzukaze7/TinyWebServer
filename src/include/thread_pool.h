#pragma once
#include "container.hpp"
#include "memory_pool.hpp"
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace suzukaze {
class ThreadPool {
public:
    using Task = std::function<void()>;

private:
    const std::size_t THREAD_COUNT_;
    std::atomic_bool stop_;
    std::queue<Task, DList<Task>> que_;
    std::mutex lock_;
    std::condition_variable cv_;
    std::vector<std::jthread> threads_;

    void work();

public:
    explicit ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency());
    ThreadPool(ThreadPool &) = delete;

    ~ThreadPool();

    void submit(Task &&task);
};
} // namespace suzukaze