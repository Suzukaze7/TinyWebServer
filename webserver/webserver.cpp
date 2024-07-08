#include "webserver.h"
#include "../exception/exception.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace suzukaze {
WebServer::~WebServer() {
    close(listen_fd);
    close(epoll_fd);
}

auto WebServer::error() -> std::string { return strerror(errno); }

void WebServer::add_fd(fd_t fd, bool in, bool one_shot) {
    epoll_event event;
    if (in)
        event.events = EPOLLIN | EPOLLET;
    else
        event.events = EPOLLOUT | EPOLLET;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event))
        throw SocketException("WebServer::add_fd epoll_ctl", error());
}

void WebServer::set_nonblock(fd_t fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

auto WebServer::read(fd_t fd) -> std::string {
    static constexpr std::size_t LEN = 1500;
    static char buf[LEN];

    std::string res;
    int cnt;
    while ((cnt = ::read(fd, buf, LEN)) > 0)
        res += buf, debug(cnt);
    return res;
}

void WebServer::create_listen() {
    addrinfo hint{}, *result;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
    err_t err;
    if ((err = getaddrinfo(nullptr, "8080", &hint, &result)))
        throw SocketException("WebServer::create_listen getaddrinfo", gai_strerror(err));

    if ((listen_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
        throw SocketException("WebServer::create_listen socket", error());

    if (bind(listen_fd, result->ai_addr, result->ai_addrlen) == -1)
        throw SocketException("WebServer::create_listen bind", error());

    if (listen(listen_fd, 1024) == -1)
        throw SocketException("WebServer::create_listen listen", error());

    set_nonblock(listen_fd);
}

void WebServer::accept() {
    fd_t fd;
    while ((fd = ::accept(listen_fd, nullptr, nullptr)) != -1) {
        set_nonblock(fd);
        add_fd(fd, true, true);
    }
}

void WebServer::exec_cmd() {
    std::string cmd;
    std::getline(std::cin, cmd);
    if (cmd.size())
        std::cout << cmd << std::endl;
}

void WebServer::receive(fd_t fd) {}

void WebServer::start_server() {
    create_listen();

    epoll_fd = epoll_create(EVENT_COUNT);
    add_fd(listen_fd, true, false);
    add_fd(STDIN_FILENO, true, false);

    while (true) {
        size_t cnt = epoll_wait(epoll_fd, events, EVENT_COUNT, -1);
        for (size_t i = 0; i < cnt; i++) {
            uint32_t event = events[i].events;
            fd_t fd = events[i].data.fd;

            if (fd == listen_fd)
                accept();
            else if (fd == STDIN_FILENO)
                exec_cmd();
            else if (event & EPOLLIN) {
                receive(fd);
            }
        }
    }
}
} // namespace suzukaze