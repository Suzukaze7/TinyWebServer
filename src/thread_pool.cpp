#include "include/thread_pool.h"
#include <utility>

namespace suzukaze {
ThreadPool::ThreadPool(std::size_t thread_count) : THREAD_COUNT_(thread_count), stop_() {
    try {
        for (std::size_t i = 0; i < THREAD_COUNT_; i++)
            threads_.emplace_back(&ThreadPool::work, this);
    } catch (...) {
        stop_ = true;
        cv_.notify_all();
        throw;
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    cv_.notify_all();
}

void ThreadPool::work() {
    while (!stop_) {
        std::unique_lock guard(lock_);
        cv_.wait(guard, [&] { return !que_.empty() || stop_; });

        if (stop_)
            break;

        auto task = std::move(que_.front());
        que_.pop();
        guard.unlock();
        task();
    }
}

void ThreadPool::submit(Task &&task) {
    {
        std::lock_guard guard(lock_);
        que_.push(std::move(task));
    }
    cv_.notify_one();
}
} // namespace suzukaze