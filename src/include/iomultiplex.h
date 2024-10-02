#include "descriptor.h"
#include <cstddef>
#include <memory>

#if __linux
#include <sys/epoll.h>
#elif _WIN32
#endif

namespace suzukaze {
class IOMultiplex {
#if __linux
    using fd_t = int;
    using event_t = epoll_event;
#elif _WIN32
#endif

public:
    class Iterator;

    class Event {
        friend class Iterator;

        event_t *ptr_;

        explicit Event(event_t *ptr) noexcept : ptr_(ptr) {}

    public:
        DescriptorBase fd() const noexcept {
#if __linux
            return DescriptorBase{ptr_->data.fd};
#elif _WIN32
#endif
        }

        bool done() const noexcept {
#if __linux
            return ptr_->events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR);
#elif _WIN32
#endif
        }

        bool is_read() const noexcept {
#if __linux
            return ptr_->events & EPOLLIN;
#elif _WIN32
#endif
        }

        bool is_write() const noexcept {
#if __linux
            return ptr_->events & EPOLLOUT;
#elif _WIN32
#endif
        }
    };

    class Iterator {
        friend class IOMultiplex;

        event_t *ptr_;

        explicit Iterator(event_t *ptr) noexcept : ptr_(ptr) {}

    public:
        bool operator==(const Iterator &) const = default;

        Iterator &operator++() noexcept {
            ptr_++;
            return *this;
        }

        auto operator*() noexcept { return Event{ptr_}; }
    };

private:
    fd_t fd_;
    std::size_t cnt_{};
    std::unique_ptr<event_t[]> events_;

public:
    IOMultiplex();
    IOMultiplex(IOMultiplex &) = delete;

    ~IOMultiplex();

    void add(const DescriptorBase &fd, bool in, bool one_shot);

    void mod(const DescriptorBase &fd, bool in);

    void del(const DescriptorBase &fd);

    Iterator begin() noexcept;

    Iterator end() noexcept;

    IOMultiplex &operator()();
};
} // namespace suzukaze