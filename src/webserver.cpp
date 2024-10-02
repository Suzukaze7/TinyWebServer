#include "include/webserver.h"
#include "include/descriptor.h"
#include "include/tool.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace suzukaze {
using namespace std::string_view_literals;

void WebServer::add(RequestMethod type, std::string_view url, Handler handler) {
    config_->router_.add_handler(type, url, std::move(handler));
}

void WebServer::get(std::string_view url, Handler handler) {
    add(RequestMethod::GET, url, std::move(handler));
}

void WebServer::post(std::string_view url, Handler handler) {
    add(RequestMethod::POST, url, std::move(handler));
}

void WebServer::exec_cmd() {
    std::string cmd;
    std::getline(std::cin, cmd);
    if (cmd == "stop"sv)
        std::exit(0);
}

void WebServer::accept_conn() {
    while (true) {
        auto [fd, host] = listen_fd_.accept();
        if (tool_->cur_conn_ == config_->max_conn_cnt_)
            continue;

        if (!fd)
            break;

        std::size_t idx = fd;
        if (idx >= conn_.size())
            conn_.resize(idx + 1);

        auto task_ptr = tool_->timer_wheel_.add_task([&conn = conn_[idx]] {
            conn->task_ptr_ = {};
            conn.reset();
        });
        conn_[idx] = std::make_unique<HttpConn>(config_.get(), tool_.get(), std::move(fd),
                                                std::move(host), task_ptr);
    }
}

void WebServer::init() {
    tool_ = std::make_unique<Tool>();

    listen_fd_.listen();
    tool_->iomultiplex_.add(listen_fd_, true, false);
    // iomultiplex_.add(Descriptor::STDIN, true, false);
    // iomultiplex_.add(tool_->timer_wheel_.read_fd(), true, false);
}

void WebServer::start_server() {
    init();

    tool_->logger_.info("listen {}:{}", config_->ip_, config_->port_);

    while (true) {
        for (auto event : tool_->iomultiplex_()) {
            auto fd = event.fd();

            tool_->logger_.debug("fd:{} r:{} w:{} done:{}", static_cast<std::size_t>(fd),
                                 event.is_read(), event.is_write(), event.done());

            if (fd == listen_fd_)
                accept_conn();
            else if (fd == Descriptor::STDIN)
                exec_cmd();
            else if (fd == tool_->timer_wheel_.read_fd())
                tool_->timer_wheel_.solve_task();
            else if (event.done())
                conn_[fd].reset();
            else if (event.is_read()) {
                if (!conn_[fd]->receive())
                    conn_[fd].reset();
            } else if (event.is_write()) {
                if (!conn_[fd]->send())
                    conn_[fd].reset();
            }
        }
    }
}
} // namespace suzukaze