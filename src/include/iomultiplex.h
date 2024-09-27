#include "socket.h"
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
    class Iterator {
        friend class IOMultiplex;

        std::size_t idx_;

        explicit Iterator(std::size_t idx) noexcept : idx_(idx) {}

    public:
        bool operator==(const Iterator &) const = default;

        Iterator &operator++() noexcept {
            idx_++;
            return *this;
        }
    };

private:
    fd_t fd_;
    std::size_t cnt_;
    std::unique_ptr<event_t[]> events_;

public:
    IOMultiplex();

    ~IOMultiplex();

    void add(const Descriptor &desc, bool in, bool one_shot);

    void mod(const Descriptor &desc, bool in);

    void del(const Descriptor &desc);

    Iterator begin() const;

    Iterator end() const;

    IOMultiplex &operator()();
};
} // namespace suzukaze