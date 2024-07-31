#pragma once
#include "logger.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace suzukaze {

template <typename T>
class MemoryPool {
    std::size_t size_;
    T *pool_;
    std::size_t *stk_;
    std::size_t top_;
    std::vector<bool> book_;

    template <typename U>
    auto alloc() -> U * {
        return std::allocator<U>().allocate(size_);
    }

    template <typename U>
    void dealloc(U *ptr) {
        std::allocator<U>().deallocate(ptr, size_);
    }

    void clear() noexcept {
        if (pool_)
            dealloc(pool_);
        if (stk_)
            dealloc(stk_);
        size_ = top_ = 0;
        pool_ = nullptr, stk_ = nullptr;
    }

public:
    MemoryPool() : size_(0), pool_(nullptr), stk_(nullptr), top_(0) {}

    MemoryPool(std::size_t n)
        : size_(n), pool_(alloc<T>()), stk_(alloc<std::size_t>()), top_(), book_(n) {
        for (; top_ < size_; top_++)
            stk_[top_] = top_;
    }

    MemoryPool(MemoryPool &&oth) noexcept {
        clear();
        swap(oth);
    }

    ~MemoryPool() {
        for (std::size_t i = 0; i < size_; i++)
            book_[i] = true;
        for (std::size_t i = 0; i < size_; i++)
            if (!book_[i])
                std::destroy_at(pool_ + i);
        clear();
    }

    void swap(MemoryPool &oth) noexcept {
        std::swap(size_, oth.size_);
        std::swap(pool_, oth.pool_);
        std::swap(stk_, oth.stk_);
        std::swap(top_, oth.top_);
        book_.swap(oth.book_);
    }

    MemoryPool &operator=(MemoryPool oth) noexcept {
        swap(oth);
        return *this;
    }

    auto empty() noexcept -> bool { return !top_; }

    template <typename... Args>
    T *allocate(Args &&...args) {
        auto ptr = std::construct_at(pool_ + stk_[top_ - 1], std::forward<Args>(args)...);
        top_--;
        return ptr;
    }

    void deallocate(T *ptr) noexcept {
        std::destroy_at(ptr);
        stk_[top_++] = ptr - pool_;
    }
};
} // namespace suzukaze