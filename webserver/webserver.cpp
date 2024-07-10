#include "webserver.h"
#include "../exception/exception.h"
#include "../type/type.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
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
#include <random>

namespace suzukaze {
WebServer::~WebServer() {
    close(listen_fd);
    close(epoll_fd);
}

auto WebServer::error() -> std::string { return strerror(errno); }

void WebServer::add_fd(fd_t fd, bool in, bool one_shot) {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event))
        throw SocketException("WebServer::add_fd epoll_ctl", error());
}

void WebServer::mod_fd(fd_t fd, bool in) {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event))
        throw SocketException("WebServer::mod_fd epoll_ctl", error());
}

void WebServer::del_fd(fd_t fd) { epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr); }

void WebServer::set_nonblock(fd_t fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

void WebServer::create_listen() {
    addrinfo hint{}, *result;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
    err_t err;

    std::default_random_engine e(time(nullptr));
    std::uniform_int_distribution<> d(1025, 65535);
    std::string port = std::to_string(d(e));
    logger.info("listen localhost:{}", port);

    if ((err = getaddrinfo(nullptr, port.c_str(), &hint, &result)))
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
    sockaddr_in sa;
    socklen_t salen = sizeof sa;
    while ((fd = ::accept(listen_fd, reinterpret_cast<sockaddr *>(&sa), &salen)) != -1) {
        constexpr size_t LEN = 20;
        char host[LEN], serv[LEN];
        getnameinfo(reinterpret_cast<sockaddr *>(&sa), salen, host, LEN, serv, LEN, NI_NUMERICHOST | NI_NUMERICSERV);
        logger.info("accept {}:{} fd: {}", host, serv, fd);

        set_nonblock(fd);
        add_fd(fd, true, true);
    }
}

void WebServer::exec_cmd() {
    std::string cmd;
    std::getline(std::cin, cmd);
    if (cmd == "stop")
        std::exit(0);
    else if (cmd.size())
        std::cout << cmd << std::endl;
}

void WebServer::receive(fd_t fd) {
    logger.info("receive fd: {}", fd);

    while (conn.size() <= fd)
        conn.emplace_back(conn.size());
    if (conn[fd].receive()) {
        pool.submit([&] {
            if (conn[fd].parse_request()) {
                conn[fd].process();
                mod_fd(fd, false);
            } else
                mod_fd(fd, true);
        });
    } else
        mod_fd(fd, true);
}

void WebServer::send(fd_t fd) {
    logger.info("send fd: {}", fd);

    if (conn[fd].send()) {
        logger.info("close {}", fd);
        del_fd(fd);
        close(fd);
    }
}

void WebServer::start_server() {
    epoll_fd = epoll_create(EVENT_COUNT);
    create_listen();
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
            else if (event & EPOLLHUP)
                del_fd(fd);
            else if (event & EPOLLIN) {
                receive(fd);
            } else if (event & EPOLLOUT) {
                send(fd);
            }
        }
    }
}
} // namespace suzukaze