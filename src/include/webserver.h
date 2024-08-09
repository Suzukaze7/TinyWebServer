#pragma once
#include "http_conn.h"
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "memory_pool.hpp"
#include "router.h"
#include "thread_pool.h"
#include "timer_wheel.h"
#include "type.h"
#include <cstddef>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <utility>
#include <vector>

namespace suzukaze {
class WebServer {
    static constexpr int EVENT_CNT_ = 10000;

    std::size_t max_conn_cnt_, cur_conn_{};
    std::string ip_;
    std::uint16_t port_;
    fd_t listen_fd_;
    fd_t epoll_fd_;
    Logger logger_;
    RootRouter router_;
    TimerWheel timer_wheel_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<epoll_event[]> events_;
    std::vector<std::unique_ptr<HttpConn>> conn_;

    void add_fd(fd_t fd, bool in, bool one_shot) noexcept;
    void mod_fd(fd_t fd, bool in) noexcept;
    void create_listen() noexcept;
    void init_resource() noexcept;
    void exec_cmd() noexcept;
    void accept_conn() noexcept;
    void close_conn(fd_t fd, bool erase_task = true) noexcept;
    void reset_conn(fd_t fd) noexcept;
    void receive_msg(fd_t fd) noexcept;
    void send_msg(fd_t fd) noexcept;

public:
    WebServer(std::string ip, std::uint16_t port, std::string static_dir = "./static",
              std::size_t max_conn_cnt = -1) noexcept
        : max_conn_cnt_(max_conn_cnt), ip_(std::move(ip)), port_(port),
          router_(std::move(static_dir)) {}

    ~WebServer() {
        close(listen_fd_);
        close(epoll_fd_);
    }

    void add(RequestMethod type, std::string_view url, Handler handler);
    void get(std::string_view url, Handler handler);
    void post(std::string_view url, Handler handler);
    void start_server() noexcept;
};
} // namespace suzukaze
