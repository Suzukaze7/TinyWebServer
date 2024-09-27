#include "include/iomultiplex.h"
#include "include/config.h"
#include <memory>

#if __linux
#include <sys/epoll.h>
#include <unistd.h>
#elif _WIN32
#endif

namespace suzukaze {
IOMultiplex::IOMultiplex() : events_(std::make_unique_for_overwrite<event_t[]>(EVENT_CNT_)) {
#if __linux
    fd_ = epoll_create(EVENT_CNT_);
#elif _WIN32
#endif
}

IOMultiplex::~IOMultiplex() {
#if __linux
    close(fd_);
#elif _WIN32
#endif
}

void IOMultiplex::add(const Descriptor &desc, bool in, bool one_shot) {
#if __linux
    epoll_event event = {.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET, .data = {.fd = desc.fd_}};
    if (one_shot) [[likely]]
        event.events |= EPOLLONESHOT;
    epoll_ctl(fd_, EPOLL_CTL_MOD, desc.fd_, &event);
#elif _WIN32
#endif
}

void IOMultiplex::mod(const Descriptor &desc, bool in) {
#if __linux
    epoll_event event = {.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT,
                         .data = {.fd = desc.fd_}};
    epoll_ctl(fd_, EPOLL_CTL_MOD, desc.fd_, &event);
#elif _WIN32
#endif
}

void IOMultiplex::del(const Descriptor &desc) {
#if __linux
    epoll_ctl(fd_, EPOLL_CTL_DEL, desc.fd_, nullptr);
#elif _WIN32
#endif
}

IOMultiplex::Iterator IOMultiplex::begin() const { return Iterator{0}; }

IOMultiplex::Iterator IOMultiplex::end() const { return Iterator{cnt_}; }

IOMultiplex &IOMultiplex::operator()() {
#if __linux
    cnt_ = epoll_wait(fd_, events_.get(), EVENT_CNT_, -1);
#elif _WIN32
#endif

    return *this;
}

} // namespace suzukaze