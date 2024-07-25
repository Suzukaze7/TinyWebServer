#pragma once
#include "http_conn.h"
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "router.h"
#include "thread_pool.hpp"
#include "type.h"
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <vector>

namespace suzukaze {
class WebServer {
    static constexpr std::size_t EVENT_COUNT = 10000;

    Logger logger_;
    Router &router_ = Router::get_instance();
    std::unique_ptr<epoll_event[]> events_ =
        std::make_unique_for_overwrite<epoll_event[]>(EVENT_COUNT);
    std::unique_ptr<ThreadPool<>> pool_ = std::make_unique<ThreadPool<>>();
    std::vector<std::unique_ptr<HttpConn>> conn_;
    std::string ip_;
    std::uint16_t port_;
    fd_t listen_fd_;
    fd_t epoll_fd_;

    void add_fd(fd_t fd, bool in, bool one_shot);
    void mod_fd(fd_t fd, bool in);
    void set_nonblock(fd_t fd);
    void create_listen();
    void exec_cmd();
    void accept_conn();
    void receive_msg(fd_t fd);
    void reset_conn(fd_t fd);
    void close_conn(fd_t fd);
    void send_msg(fd_t fd);

public:
    WebServer(std::string ip, std::uint16_t port) noexcept : ip_(std::move(ip)), port_(port) {}

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
