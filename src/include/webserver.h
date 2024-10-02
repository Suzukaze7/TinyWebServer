#pragma once
#include "config.h"
#include "descriptor.h"
#include "http_conn.h"
#include "http_request.h"
#include "router.h"
#include "tool.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace suzukaze {
class WebServer {
    Descriptor listen_fd_;
    std::unique_ptr<Config> config_;
    std::unique_ptr<Tool> tool_;
    std::vector<std::unique_ptr<HttpConn>> conn_;

    void init();
    void exec_cmd();
    void accept_conn();

public:
    WebServer(std::string ip, std::uint16_t port, std::string static_dir = "../static/",
              std::size_t max_conn_cnt = -1)
        : listen_fd_(ip, port), config_(std::make_unique<Config>(max_conn_cnt, std::move(ip), port,
                                                                 std::move(static_dir))) {}

    void add(RequestMethod type, std::string_view url, Handler handler);
    void get(std::string_view url, Handler handler);
    void post(std::string_view url, Handler handler);
    void start_server();
};
} // namespace suzukaze
