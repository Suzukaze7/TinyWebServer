#include "include/descriptor.h"
#include "include/config.h"
#include "include/exception.h"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#if __linux
#include <fcntl.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#elif _WIN32
#endif

namespace suzukaze {
std::pair<Descriptor, Descriptor> Descriptor::pipe() {
#if __linux
    fd_t fd[2];
    ::pipe(fd);
    return {Descriptor{fd[0]}, Descriptor{fd[1]}};
#elif _WIN32
#endif
}

Descriptor::Descriptor(std::string_view path, OFlag flag) {
#if __linux
    if ((fd_ = open(path.data(), flag)) == -1)
        throw SysCallException("Descriptor::Descriptor");
#elif _WIN32
#endif
}

void Descriptor::set_nonblock(fd_t fd) {
#if __linux
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
#elif _WIN32
#endif
}

bool Descriptor::read(std::string &s) {
    constexpr std::size_t N = 10000;
    static char buf[N];
#if __linux
    int cnt;
    while ((cnt = ::read(fd_, buf, N)) > 0)
        s.append(buf, cnt);
    return cnt == -1;
#elif _WIN32
#endif
}

void Descriptor::write(std::string_view data) {
#if __linux
    ::write(fd_, data.data(), data.size());
#elif _WIN32
#endif
}

Descriptor::fd_t Descriptor::bind_socket(std::string_view ip, std::uint16_t port) {
#if __linux
    addrinfo hint{}, *res;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG;
    ::getaddrinfo(nullptr, std::to_string(port).c_str(), &hint, &res);
    std::unique_ptr<addrinfo, decltype([](addrinfo *ai) { freeaddrinfo(ai); })> guard(res);
    fd_t fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    assert(fd != -1);

    ::bind(fd, res->ai_addr, res->ai_addrlen);
    set_nonblock(fd);
    return fd;
#elif _WIN32
#endif
}

void Descriptor::listen() {
#if __linux
    int flag = true;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ::listen(fd_, EVENT_CNT_);
#elif _WIN32
#endif
}

std::pair<Descriptor, std::string> Descriptor::accept() {
#if __linux
    sockaddr sa;
    socklen_t salen = sizeof(sa);
    fd_t fd = ::accept(fd_, &sa, &salen);

    constexpr std::size_t LEN = 50;
    char host[LEN], serv[LEN];
    getnameinfo(&sa, salen, host, LEN, serv, LEN, NI_NUMERICHOST | NI_NUMERICSERV);

    return {Descriptor{fd}, std::format("{}:{}", host, serv)};
#elif _WIN32
#endif
}

MMap::MMap(std::string_view path) {
    Descriptor fd(path, {.read_ = 1});
    size_ = std::filesystem::file_size(path);
#if __linux
    if ((ptr_ = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd.fd_, 0)) == MAP_FAILED)
        throw SysCallException("MMap::MMap mmap");
#elif _WIN32
#endif
}

MMap::~MMap() {
#if __linux
    if (ptr_) [[likely]]
        ::munmap(ptr_, size_);
#elif _WIN32
#endif
}
} // namespace suzukaze