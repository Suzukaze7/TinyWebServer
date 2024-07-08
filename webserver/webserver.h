#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <vector>
#include "../http_conn/http_conn.h"

namespace suzukaze {
using fd_t = int;
using err_t = int;
using size_t = int;

class WebServer {
    static constexpr size_t EVENT_COUNT = 10000;
    fd_t listen_fd;
    fd_t epoll_fd;
    epoll_event events[EVENT_COUNT];
    std::vector<HttpConn> conn;

    auto error() -> std::string;
    void add_fd(fd_t fd, bool in, bool one_shot);
    void set_nonblock(fd_t fd);
    auto read(fd_t fd) -> std::string;
    void create_listen();
    void exec_cmd();
    void accept();
    void receive(fd_t fd);

public:
    ~WebServer();

    void start_server();
};
} // namespace suzukaze
