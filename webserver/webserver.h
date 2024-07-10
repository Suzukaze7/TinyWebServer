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
    Logger logger;

    auto error() -> std::string;
    void add_fd(fd_t fd, bool in, bool one_shot);
    void mod_fd(fd_t fd, bool in);
    void del_fd(fd_t fd);
    void set_nonblock(fd_t fd);
    auto read(fd_t fd) -> std::string;
    void create_listen();
    void exec_cmd();
    void accept();
    void receive(fd_t fd);
    void send(fd_t fd);

public:
    ~WebServer();

    void start_server();
};
} // namespace suzukaze
