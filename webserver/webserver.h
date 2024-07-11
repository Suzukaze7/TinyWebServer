#pragma once
#include "../http_conn/http_conn.h"
#include "../logger/logger.hpp"
#include "../thread_pool/thread_pool.hpp"
#include "../type/type.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <vector>

namespace suzukaze {
class WebServer {
    static constexpr size_t EVENT_COUNT = 10000;
    fd_t listen_fd;
    fd_t epoll_fd;
    epoll_event events[EVENT_COUNT];

    ThreadPool<> pool;
    std::vector<HttpConn> conn;

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
    ~WebServer();

    void start_server();
};
} // namespace suzukaze
