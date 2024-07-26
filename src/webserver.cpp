#include "include/webserver.h"
#include "include/exception.h"
#include "include/http_conn.h"
#include "include/type.h"
#include "include/utils.h"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace suzukaze {
void WebServer::add(RequestMethod type, std::string_view url, Handler handler) {
    router_.add_handler(type, url, std::move(handler));
}

void WebServer::get(std::string_view url, Handler handler) {
    add(RequestMethod::GET, url, std::move(handler));
}

void WebServer::post(std::string_view url, Handler handler) {
    add(RequestMethod::POST, url, std::move(handler));
}

void WebServer::add_fd(fd_t fd, bool in, bool one_shot) {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event))
        throw SocketException("WebServer::add_fd epoll_ctl", error());
}

void WebServer::mod_fd(fd_t fd, bool in) {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event))
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
    if ((err = getaddrinfo(nullptr, std::to_string(port_).c_str(), &hint, &result)))
        throw SocketException("WebServer::create_listen getaddrinfo", gai_strerror(err));

    if ((listen_fd_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
        throw SocketException("WebServer::create_listen socket", error());

    if (bind(listen_fd_, result->ai_addr, result->ai_addrlen) == -1)
        throw SocketException("WebServer::create_listen bind", error());

    if (listen(listen_fd_, 1024) == -1)
        throw SocketException("WebServer::create_listen listen", error());

    set_nonblock(listen_fd_);
}

void WebServer::accept_conn() {
    fd_t fd;
    sockaddr_in sa;
    socklen_t salen = sizeof sa;
    while ((fd = ::accept(listen_fd_, reinterpret_cast<sockaddr *>(&sa), &salen)) != -1) {
        constexpr std::size_t LEN = 20;
        char host[LEN], serv[LEN];
        getnameinfo(reinterpret_cast<sockaddr *>(&sa), salen, host, LEN, serv, LEN,
                    NI_NUMERICHOST | NI_NUMERICSERV);
        logger_.debug("accept {}:{} fd: {}", host, serv, fd);

        while (static_cast<std::size_t>(fd) + 1 > conn_.size())
            conn_.push_back(std::make_unique<HttpConn>(conn_.size()));

        set_nonblock(fd);
        add_fd(fd, true, true);
    }
}

void WebServer::reset_conn(fd_t fd) {
    conn_[fd]->~HttpConn();
    new (conn_[fd].get()) HttpConn(fd);
}

void WebServer::close_conn(fd_t fd) {
    logger_.debug("close fd: {}", fd);

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    reset_conn(fd);
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
    logger_.debug("receive fd: {}", fd);

    if (conn_[fd]->receive()) {
        close_conn(fd);
        return;
    }

    pool_->submit([fd, this, &conn = *conn_[fd]] {
        if (conn.parse_request() == ParseStatus::NOT_FINISH)
            mod_fd(fd, true);
        else {
            conn.process();
            mod_fd(fd, false);
        }
    });
}

void WebServer::send_msg(fd_t fd) {
    logger_.debug("send fd: {}", fd);

    switch (conn_[fd]->send()) {
    case SendStatus::NOT_FINISH:
        mod_fd(fd, false);
        break;
    case SendStatus::CLOSE:
        close_conn(fd);
        break;
    case SendStatus::KEEP_ALIVE:
        reset_conn(fd);
        mod_fd(fd, true);
        break;
    }
}

void WebServer::start_server() noexcept {
    epoll_fd_ = epoll_create(EVENT_COUNT);
    create_listen();
    add_fd(listen_fd_, true, false);
    add_fd(STDIN_FILENO, true, false);

    logger_.info("listen {}:{}", ip_, port_);

    while (true) {
        ssize_t cnt = epoll_wait(epoll_fd_, events_.get(), EVENT_COUNT, -1);
        for (ssize_t i = 0; i < cnt; i++) {
            auto event = events_[i].events;
            fd_t fd = events_[i].data.fd;

            logger_.debug("event: {}", event);

            if (fd == listen_fd_)
                accept_conn();
            else if (fd == STDIN_FILENO)
                exec_cmd();
            else if (event & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
                close_conn(fd);
            else if (event & EPOLLIN)
                receive_msg(fd);
            else if (event & EPOLLOUT)
                send_msg(fd);
        }
    }
}
} // namespace suzukaze