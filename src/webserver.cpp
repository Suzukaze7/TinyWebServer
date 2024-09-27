#include "include/webserver.h"
#include "include/exception.h"
#include "include/http_conn.h"
#include "include/type.h"
#include "include/utils.h"
#include <asm-generic/socket.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace suzukaze {
using namespace std::string_view_literals;

void WebServer::add(RequestMethod type, std::string_view url, Handler handler) {
    router_.add_handler(type, url, std::move(handler));
}

void WebServer::get(std::string_view url, Handler handler) {
    add(RequestMethod::GET, url, std::move(handler));
}

void WebServer::post(std::string_view url, Handler handler) {
    add(RequestMethod::POST, url, std::move(handler));
}

void WebServer::add_fd(fd_t fd, bool in, bool one_shot) noexcept {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    event.data.fd = fd;

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

void WebServer::mod_fd(fd_t fd, bool in) noexcept {
    epoll_event event;
    event.events = (in ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;

    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

void WebServer::init_resource() noexcept {
    create_listen();
    epoll_fd_ = epoll_create(EVENT_CNT_);
    add_fd(listen_fd_, true, false);
    add_fd(STDIN_FILENO, true, false);
    add_fd(timer_wheel_.read_fd(), true, false);

    thread_pool_ = std::make_unique<decltype(thread_pool_)::element_type>();
    events_ = std::make_unique_for_overwrite<decltype(events_)::element_type[]>(EVENT_CNT_);
    timer_wheel_.start();
}

void WebServer::exec_cmd() noexcept {
    std::string cmd;
    std::getline(std::cin, cmd);
    if (cmd == "stop"sv)
        std::exit(0);
}

void WebServer::accept_conn() noexcept {
    fd_t fd;
    sockaddr_in sa;
    socklen_t salen = sizeof sa;
    while ((fd = accept(listen_fd_, reinterpret_cast<sockaddr *>(&sa), &salen)) != -1) {
        if (cur_conn_ == max_conn_cnt_) {
            close(fd);
            continue;
        }

        constexpr std::size_t LEN = 20;
        char host[LEN], serv[LEN];
        getnameinfo(reinterpret_cast<sockaddr *>(&sa), salen, host, LEN, serv, LEN,
                    NI_NUMERICHOST | NI_NUMERICSERV);
        logger_.debug("WebServer::accept_conn accept {}:{} fd: {}", host, serv, fd);

        if (static_cast<std::size_t>(fd) + 1 > conn_.size())
            conn_.resize(static_cast<std::size_t>(fd) + 1);

        auto task_ptr = timer_wheel_.add_task([&, fd] { close_conn(fd, false); });
        conn_[fd] = std::make_unique<HttpConn>(logger_, router_, task_ptr, fd,
                                               std::format("{}:{}", host, serv));
        set_nonblock(fd);
        add_fd(fd, true, true);
        cur_conn_++;
    }
}

void WebServer::close_conn(fd_t fd, bool erase_task) noexcept {
    logger_.debug("WebServer::close_conn close fd: {}", fd);

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    if (erase_task)
        timer_wheel_.erase_task(conn_[fd]->task_ptr_);
    conn_[fd].reset();
    cur_conn_--;
}

void WebServer::reset_conn(fd_t fd) noexcept {
    logger_.debug("WebServer::reset_conn reset fd: {}", fd);

    conn_[fd]->clear();
    mod_fd(fd, true);
}

void WebServer::receive_msg(fd_t fd) noexcept {
    if (conn_[fd]->receive()) {
        close_conn(fd);
        return;
    }

    timer_wheel_.modify_task(conn_[fd]->task_ptr_);
    thread_pool_->submit([fd, this, &conn = *conn_[fd]] {
        if (conn.parse_request() == ParseStatus::NOT_FINISH)
            mod_fd(fd, true);
        else {
            conn.process();
            mod_fd(fd, false);
        }
    });
}

void WebServer::send_msg(fd_t fd) noexcept {
    logger_.debug("WebServer::send_msg send fd: {}", fd);

    switch (conn_[fd]->send()) {
    case SendStatus::NOT_FINISH:
        mod_fd(fd, false);
        break;
    case SendStatus::CLOSE:
        close_conn(fd);
        return;
    case SendStatus::KEEP_ALIVE:
        reset_conn(fd);
        break;
    }
    timer_wheel_.modify_task(conn_[fd]->task_ptr_);
}

void WebServer::start_server() noexcept {
    init_resource();

    logger_.info("listen {}:{}", ip_, port_);

    while (true) {
        ssize_t cnt = epoll_wait(epoll_fd_, events_.get(), EVENT_CNT_, -1);
        for (ssize_t i = 0; i < cnt; i++) {
            auto event = events_[i].events;
            fd_t fd = events_[i].data.fd;

            logger_.debug("event: {} fd: {}", event, fd);

            if (fd == listen_fd_)
                accept_conn();
            else if (fd == STDIN_FILENO)
                exec_cmd();
            else if (fd == timer_wheel_.read_fd())
                timer_wheel_.solve_task();
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