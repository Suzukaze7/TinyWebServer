#include "include/iomultiplex.h"
#include "include/config.h"
#include <cassert>
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

void IOMultiplex::add(const DescriptorBase &fd, bool in, bool one_shot) {
#if __linux
    epoll_event event = {.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET, .data = {.fd = fd.fd_}};
    if (one_shot) [[likely]]
        event.events |= EPOLLONESHOT;
    epoll_ctl(fd_, EPOLL_CTL_ADD, fd.fd_, &event);
#elif _WIN32
#endif
}

void IOMultiplex::mod(const DescriptorBase &fd, bool in) {
#if __linux
    epoll_event event = {.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT,
                         .data = {.fd = fd.fd_}};
    epoll_ctl(fd_, EPOLL_CTL_MOD, fd.fd_, &event);
#elif _WIN32
#endif
}

void IOMultiplex::del(const DescriptorBase &fd) {
#if __linux
    epoll_ctl(fd_, EPOLL_CTL_DEL, fd.fd_, nullptr);
#elif _WIN32
#endif
}

IOMultiplex::Iterator IOMultiplex::begin() noexcept { return Iterator{events_.get()}; }

IOMultiplex::Iterator IOMultiplex::end() noexcept { return Iterator{events_.get() + cnt_}; }

IOMultiplex &IOMultiplex::operator()() {
#if __linux
    cnt_ = epoll_wait(fd_, events_.get(), EVENT_CNT_, -1);
#elif _WIN32
#endif

    return *this;
}
} // namespace suzukaze