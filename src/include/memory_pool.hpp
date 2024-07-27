#include "logger.hpp"
#include <cstddef>
#include <deque>
#include <memory>
#include <numeric>
#include <tuple>
#include <utility>

namespace suzukaze {

template <typename T>
class MemoryPool {
    std::size_t size_;
    T *pool_;
    std::size_t *stk_;
    std::size_t top_;

    void clear() noexcept {
        if (size_) {
            std::allocator<T>().deallocate(pool_, size_);
            std::allocator<std::size_t>().deallocate(stk_, size_);
            size_ = top_ = 0;
            pool_ = nullptr, stk_ = nullptr;
        }
    }

public:
    MemoryPool() : size_(0), pool_(nullptr), stk_(nullptr), top_(0) {}

    MemoryPool(std::size_t n)
        : size_(n), pool_(std::allocator<T>().allocate(n)),
          stk_(std::allocator<std::size_t>().allocate(n)), top_(n) {
        std::iota(stk_, stk_ + n, static_cast<size_t>(0));
    }

    MemoryPool(MemoryPool &&oth) noexcept {
        clear();
        swap(oth);
    }

    ~MemoryPool() { clear(); }

    void swap(MemoryPool &oth) noexcept {
        std::swap(size_, oth.size_);
        std::swap(pool_, oth.pool_);
        std::swap(stk_, oth.stk_);
        std::swap(top_, oth.top_);
    }

    MemoryPool &operator=(MemoryPool oth) noexcept {
        swap(oth);
        return *this;
    }

    bool empty() noexcept { return !top_; }

    template <typename... Args>
    T *allocate(Args &&...args) noexcept {
        auto p = pool_ + stk_[--top_];
        std::construct_at(p, std::forward<Args>(args)...);
        return p;
    }

    void deallocate(T *ptr) noexcept {
        std::destroy_at(ptr);
        stk_[top_++] = ptr - pool_;
    }
};
} // namespace suzukaze