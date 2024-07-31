#pragma once
#include "memory_pool.hpp"
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace suzukaze {
template <typename T>
concept Task =
    std::invocable<T> && std::movable<T> && std::is_default_constructible_v<T>; // Queue need

template <typename T>
class Queue {
    struct Node {
        T data_;
        Node *next_;
    };

    Node head_, *tail_;
    MemoryPool<Node> mem_pool_;

public:
    Queue(std::size_t max_size) : head_(), tail_(&head_), mem_pool_(max_size) {}

    // ~Queue(); 不需要析构，因为 MemoryPool 会自动回收

    auto empty() noexcept -> bool { return !head_.next_; }

    void push(T &&data) { tail_ = tail_->next_ = mem_pool_.allocate(std::move(data)); }

    T pop() {
        auto data = std::move(head_.next_->data_);
        auto next = head_.next_->next_;
        mem_pool_.deallocate(head_.next_);
        head_.next_ = next;
        if (empty())
            tail_ = &head_;
        return data; // nrvo?
    }
};

template <Task T = std::function<void()>>
class ThreadPool {
    const std::size_t THREAD_COUNT_;
    std::atomic_bool stop_;
    Queue<T> que_;
    std::mutex lock_;
    std::condition_variable cv_;
    std::vector<std::jthread> threads_;

    void work() {
        while (!stop_) {
            std::unique_lock guard(lock_);
            cv_.wait(guard, [&] { return !que_.empty() || stop_; });

            if (stop_)
                break;

            auto task = que_.pop();
            guard.unlock();
            task();
        }
    }

public:
    ThreadPool(std::size_t max_que_size,
               std::size_t thread_count = std::thread::hardware_concurrency())
        : THREAD_COUNT_(thread_count), stop_(), que_(max_que_size) {
        try {
            for (std::size_t i = 0; i < THREAD_COUNT_; i++)
                threads_.emplace_back(&ThreadPool::work, this);
        } catch (...) {
            stop_ = true;
            cv_.notify_all();
            throw;
        }
    }

    ThreadPool(ThreadPool &) = delete;

    ~ThreadPool() {
        stop_ = true;
        cv_.notify_all();
    }

    void submit(T task) {
        {
            std::lock_guard guard(lock_);
            que_.push(std::move(task));
        }
        cv_.notify_one();
    }
};
} // namespace suzukaze