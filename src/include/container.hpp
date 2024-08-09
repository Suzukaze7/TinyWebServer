#pragma once
#include "memory_pool.hpp"
#include <cstddef>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

namespace suzukaze {
template <typename T>
class DList {
    struct DNode;

    struct Node {
        DNode *prev_, *next_;
    };

    struct DNode : public Node {
        T data_;

        void *operator new(std::size_t) { return MemoryPool<DNode>::get_instance().allocate(); }

        void operator delete(void *ptr) { MemoryPool<DNode>::get_instance().deallocate(ptr); }
    };

public:
    class iterator {
        friend class DList;

        using trait = std::iterator_traits<T *>;

    public:
        using value_type = typename trait::value_type;
        using difference_type = typename trait::difference_type;
        using iterator_category = typename trait::iterator_category;
        using pointer = typename trait::pointer;
        using reference = typename trait::reference;

    private:
        DNode *nd_{};

        iterator(DNode *nd) noexcept : nd_(nd) {}

    public:
        iterator() = default;

        bool operator==(const iterator &oth) const noexcept { return nd_ == oth.nd_; }

        reference operator*() const noexcept { return nd_->data_; }

        pointer operator->() const noexcept { return &nd_->data_; }

        iterator &operator++() {
            nd_ = nd_->next_;
            return *this;
        }

        iterator operator++(int) {
            iterator last(nd_);
            ++*this;
            return last;
        }

        iterator &operator--() {
            nd_ = nd_->prev_;
            return *this;
        }

        iterator operator--(int) {
            iterator last(nd_);
            --*this;
            return last;
        }
    };

    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using const_iterator = const iterator;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

public:
    static iterator insert(const_iterator it, const T &data) {
        DNode *nnd = it.nd_, *pnd = nnd->prev_;
        return nnd->prev_ = pnd->next_ = new DNode{pnd, nnd, data};
    }

    static iterator insert(const_iterator it, T &&data) {
        DNode *nnd = it.nd_, *pnd = nnd->prev_;
        return nnd->prev_ = pnd->next_ = new DNode{pnd, nnd, std::move(data)};
    }

    static iterator erase(iterator it) noexcept {
        DNode *nnd = it.nd_->next_, *pnd = it.nd_->prev_;
        delete it.nd_;
        nnd->prev_ = pnd, pnd->next_ = nnd;
        return nnd;
    }

private:
    Node head_, tail_;

public:
    DList() noexcept
        : head_{.next_ = reinterpret_cast<DNode *>(&tail_)},
          tail_{.prev_ = reinterpret_cast<DNode *>(&head_)} {}

    DList(DList &&oth) noexcept : DList() { swap(oth); }

    void swap(DList &oth) noexcept {
        std::swap(head_, oth.head_);
        std::swap(tail_, oth.tail_);
    }

    bool empty() const noexcept { return head_.next_ == &tail_; }

    iterator begin() noexcept { return head_.next_; }

    iterator end() noexcept { return reinterpret_cast<DNode *>(&tail_); }

    reference front() noexcept { return head_.next_->data_; }

    reference back() noexcept { return tail_.prev_->data_; }

    void push_back(const T &data) { insert(end(), data); }

    void push_back(T &&data) { insert(end(), std::move(data)); }

    void pop_front() noexcept { erase(begin()); }

    void clear() {
        for (DNode *it = head_.next_, *nit = it->next_; nit; it = nit, nit = nit->next_)
            delete it;
        head_.next_ = reinterpret_cast<DNode *>(&tail_);
        tail_.prev_ = reinterpret_cast<DNode *>(&head_);
    }
};
} // namespace suzukaze