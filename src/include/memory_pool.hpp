#pragma once
#include "logger.hpp"
#include <cstddef>
#include <deque>
#include <list>
#include <memory>
#include <stack>
#include <utility>
#include <vector>

namespace suzukaze {
template <typename T>
class MemoryPool {
    using alloc = std::allocator<T>;

    static constexpr std::size_t INCREASEMENT = 2;

    struct Chunk {
        T *pool_;
        std::size_t size_, cur_;

        Chunk() : pool_(), size_(), cur_() {}
        Chunk(std::size_t n) : pool_(alloc().allocate(n)), size_(n), cur_(0) {}
        Chunk(Chunk &&oth) : Chunk() { swap(oth); }

        ~Chunk() {
            if (pool_) {
                alloc().deallocate(pool_, size_);
                pool_ = nullptr;
                size_ = cur_ = 0;
            }
        }

        Chunk &operator=(Chunk oth) noexcept {
            swap(oth);
            return *this;
        }

        void swap(Chunk &oth) noexcept {
            std::swap(pool_, oth.pool_);
            std::swap(size_, oth.size_);
            std::swap(cur_, oth.cur_);
        }
    };

    std::list<Chunk> ls_;
    Chunk cur_chunk_;
    std::stack<T *, std::vector<T *>> stk_;

    MemoryPool() = default;
    MemoryPool(MemoryPool &) = delete;

public:
    static MemoryPool &get_instance() noexcept {
        static MemoryPool instance;
        return instance;
    }

    T *allocate() {
        if (stk_.empty()) {
            if (!cur_chunk_.size_)
                cur_chunk_ = {1}; // rvo
            else {
                ls_.push_back(std::move(cur_chunk_));
                cur_chunk_ = {ls_.back().size_ * INCREASEMENT};
            }

            auto &[pool, size, _] = cur_chunk_;
            for (std::size_t i = 0; i < size; i++)
                stk_.push(pool + i);
        }

        T *p = stk_.top();
        stk_.pop();
        return p;
    }

    void deallocate(T *ptr) noexcept {
        auto &[pool, size, _] = cur_chunk_;
        if (pool <= ptr && ptr < pool + size)
            stk_.push(ptr);
        else {
            for (auto it = ls_.begin(); it != ls_.end(); it++) {
                auto &[pool, size, cur] = *it;
                if (pool <= ptr && ptr < pool + size) {
                    if (++cur == size)
                        ls_.erase(it);
                    break;
                }
            }
        }
    }

    void deallocate(void *ptr) noexcept { deallocate(static_cast<T *>(ptr)); }
};
} // namespace suzukaze