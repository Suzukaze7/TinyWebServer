#pragma once
#include "http_conn.h"
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "memory_pool.hpp"
#include "router.h"
#include "thread_pool.hpp"
#include "type.h"
#include <cstddef>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <vector>

namespace suzukaze {
class WebServer {
    const std::size_t MAX_CONN_CNT_;
    std::string ip_;
    std::uint16_t port_;
    fd_t listen_fd_;
    fd_t epoll_fd_;
    Logger logger_;
    Router &router_ = Router::get_instance();
    std::unique_ptr<ThreadPool<>> thread_pool_;
    std::unique_ptr<epoll_event[]> events_;
    MemoryPool<HttpConn> mem_pool_;
    std::vector<HttpConn *> conn_;

    void add_fd(fd_t fd, bool in, bool one_shot);
    void mod_fd(fd_t fd, bool in);
    void set_nonblock(fd_t fd);
    void create_listen();
    void init_resource();
    void exec_cmd();
    void accept_conn();
    void receive_msg(fd_t fd);
    void close_conn(fd_t fd);
    void send_msg(fd_t fd);

public:
    WebServer(std::string ip, std::uint16_t port, std::size_t max_conn_cnt = 10000) noexcept
        : MAX_CONN_CNT_(max_conn_cnt), ip_(std::move(ip)), port_(port) {}

    ~WebServer() {
        close(listen_fd_);
        close(epoll_fd_);
    }

    void add(RequestMethod type, std::string_view url, Handler handler);
    void get(std::string_view url, Handler handler);
    void post(std::string_view url, Handler handler);
    void start_server();
};
} // namespace suzukaze
