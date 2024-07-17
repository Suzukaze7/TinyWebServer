#pragma once
#include "http_conn.h"
#include "logger.hpp"
#include "thread_pool.hpp"
#include "type.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <vector>

namespace suzukaze {
class WebServer {
    static constexpr std::size_t EVENT_COUNT = 10000;
    std::unique_ptr<epoll_event[]> events{new epoll_event[EVENT_COUNT]};
    std::unique_ptr<ThreadPool<>> pool{new ThreadPool<>};
    std::vector<HttpConn> conn;
    std::string ip;
    std::uint16_t port;
    fd_t listen_fd;
    fd_t epoll_fd;

    void add_fd(fd_t fd, bool in, bool one_shot);
    void mod_fd(fd_t fd, bool in);
    void set_nonblock(fd_t fd);
    void create_listen();
    void exec_cmd();
    void accept_conn();
    void receive_msg(fd_t fd);
    void close_conn(fd_t fd);
    void send_msg(fd_t fd);

public:
    inline WebServer(std::string ip = "0.0.0.0", std::uint16_t port = 80)
        : ip(std::move(ip)), port(port){};

    WebServer(WebServer &&) = default;

    inline ~WebServer() {
        close(listen_fd);
        close(epoll_fd);
    }

    void start_server();
};
} // namespace suzukaze
