#include "include/webserver.h"
#include "include/exception.h"
#include "include/http_conn.h"
#include "include/type.h"
#include "include/utils.h"
#include <algorithm>
#include <charconv>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace suzukaze {
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
    if ((err = getaddrinfo(nullptr, std::to_string(port).c_str(), &hint, &result)))
        throw SocketException("WebServer::create_listen getaddrinfo", gai_strerror(err));

    if ((listen_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
        throw SocketException("WebServer::create_listen socket", error());

    if (bind(listen_fd, result->ai_addr, result->ai_addrlen) == -1)
        throw SocketException("WebServer::create_listen bind", error());

    if (listen(listen_fd, 1024) == -1)
        throw SocketException("WebServer::create_listen listen", error());

    set_nonblock(listen_fd);
}

void WebServer::accept_conn() {
    fd_t fd;
    sockaddr_in sa;
    socklen_t salen = sizeof sa;
    while ((fd = ::accept(listen_fd, reinterpret_cast<sockaddr *>(&sa), &salen)) != -1) {
        constexpr std::size_t LEN = 20;
        char host[LEN], serv[LEN];
        getnameinfo(reinterpret_cast<sockaddr *>(&sa), salen, host, LEN, serv, LEN,
                    NI_NUMERICHOST | NI_NUMERICSERV);
        Logger::info("accept {}:{} fd: {}", host, serv, fd);

        conn.resize(std::max(conn.size(), static_cast<std::size_t>(fd) + 1));
        (void)new (&conn[fd]) HttpConn{fd};
        set_nonblock(fd);
        add_fd(fd, true, true);
    }
}

void WebServer::close_conn(fd_t fd) {
    Logger::debug("close {}", fd);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    conn[fd].~HttpConn();
}

void WebServer::exec_cmd() {
    std::string cmd;
    std::getline(std::cin, cmd);
    if (cmd == "stop")
        std::exit(0);
    else if (cmd.size())
        std::cout << cmd << std::endl;
}

void WebServer::receive_msg(fd_t fd) {
    Logger::debug("receive fd: {}", fd);

    conn[fd].receive_msg();
    pool->submit([&] {
        auto parse_status = conn[fd].parse_request();
        if (parse_status == ParseStatus::NOT_FINISH)
            mod_fd(fd, true);
        else {
            conn[fd].process(parse_status);
            mod_fd(fd, false);
        }
    });
}

void WebServer::send_msg(fd_t fd) {
    Logger::debug("send fd: {}", fd);

    switch (conn[fd].send_msg()) {
    case SendStatus::NOT_FINISH:
        mod_fd(fd, false);
        break;
    case SendStatus::CLOSE:
        close_conn(fd);
        break;
    case SendStatus::KEEP_ALIVE:
        conn[fd].~HttpConn();
        mod_fd(fd, true);
        break;
    }
}

void WebServer::start_server() {
    epoll_fd = epoll_create(EVENT_COUNT);
    create_listen();
    add_fd(listen_fd, true, false);
    add_fd(STDIN_FILENO, true, false);

    Logger::info("listen {}:{}", ip, port);

    while (true) {
        ssize_t cnt = epoll_wait(epoll_fd, events.get(), EVENT_COUNT, -1);
        for (ssize_t i = 0; i < cnt; i++) {
            uint32_t event = events[i].events;
            fd_t fd = events[i].data.fd;

            if (fd == listen_fd)
                accept_conn();
            else if (fd == STDIN_FILENO)
                exec_cmd();
            else if (event & EPOLLHUP)
                close_conn(fd);
            else if (event & EPOLLIN)
                receive_msg(fd);
            else if (event & EPOLLOUT)
                send_msg(fd);
        }
    }
}
} // namespace suzukaze