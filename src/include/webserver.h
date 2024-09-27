#pragma once
#include "catalog.h"
#include "http_conn.h"
#include "http_request.h"
#include "iomultiplex.h"
#include "router.h"
#include "socket.h"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace suzukaze {
class WebServer {
    std::size_t max_conn_cnt_, cur_conn_{};
    std::string ip_;
    std::uint16_t port_;
    Socket listen_sock_;
    IOMultiplex iomultiplex_;
    std::unique_ptr<Catalog> catalog_;
    std::vector<std::unique_ptr<HttpConn>> conn_;

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

    void add(RequestMethod type, std::string_view url, Handler handler);
    void get(std::string_view url, Handler handler);
    void post(std::string_view url, Handler handler);
    void start_server() noexcept;
};
} // namespace suzukaze
