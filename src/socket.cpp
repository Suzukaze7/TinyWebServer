#include "include/socket.h"
#include "include/config.h"
#include <memory>

#if __linux
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#elif _WIN32
#endif

namespace suzukaze {
Socket::Socket(std::string_view ip, std::uint16_t port) {
#if __linux
    addrinfo hint{}, *res;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG;
    ::getaddrinfo(nullptr, std::to_string(port).c_str(), &hint, &res);
    std::unique_ptr<addrinfo, decltype([](addrinfo *ai) { freeaddrinfo(ai); })> guard(res);
    fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(fd_, res->ai_addr, res->ai_addrlen);
#elif _WIN32
#endif
}

Socket::~Socket() {
#if __linux
    if (~fd_) [[likely]]
        ::close(fd_);
#elif _WIN32
#endif
}

void Socket::set_nonblock(fd_t fd) const {
#if __linux
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
#elif _WIN32
#endif
}

void Socket::listen() const {
#if __linux
    int flag = true;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
    ::listen(fd_, EVENT_CNT_);
#elif _WIN32
#endif
}

Socket Socket::accept() const {
#if __linux
    sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    return ::accept(fd_, reinterpret_cast<sockaddr *>(&sa), &salen);
#elif _WIN32
#endif
}
} // namespace suzukaze