#pragma once
#include "config.h"
#include "descriptor.h"
#include "http_request.h"
#include "http_response.h"
#include "memory_pool.hpp"
#include "timer_wheel.h"
#include "tool.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace suzukaze {

class HttpConn {
    enum class ParseStatus { CONTINUE, NOT_FINISH, FINISH };

    static inline const std::map<StatusCode, std::pair<std::uint16_t, std::string>> STATUS_INFO = {
        {StatusCode::OK, {200, "OK"}},
        {StatusCode::BAD_REQUEST, {400, "Bad Request"}},
        {StatusCode::NOT_FOUND, {404, "Not Found"}},
        {StatusCode::NOT_ALLOWED, {405, "Method Not Allowed"}},
        {StatusCode::INTERNAL_ERROR, {500, "Internal Server Error"}},
    };

public:
    void *operator new(std::size_t) { return MemoryPool<HttpConn>::get_instance().allocate(); }

    void operator delete(void *ptr) noexcept {
        MemoryPool<HttpConn>::get_instance().deallocate(ptr);
    }

public:
    TimerWheel::Pointer task_ptr_;

private:
    Config *config_;
    Tool *tool_;
    Descriptor fd_;
    const std::string host_;
    bool keep_alive_;
    RequestInfo req_;
    ResponseInfo resp_;

public:
    HttpConn(Config *config, Tool *tool, Descriptor &&fd, std::string &&host,
             TimerWheel::Pointer task_ptr) noexcept
        : task_ptr_(task_ptr), config_(config), tool_(tool), fd_(std::move(fd)),
          host_(std::move(host)) {
        tool_->iomultiplex_.add(fd_, true, true);
        tool_->cur_conn_++;
    }
    HttpConn(HttpConn &) = delete;

    ~HttpConn() {
        tool_->iomultiplex_.del(fd_);
        tool_->timer_wheel_.erase_task(task_ptr_);
        tool_->cur_conn_--;
    }

private:
    bool equal_ignore_case(std::string_view lhs, std::string_view rhs) noexcept;
    bool getline() noexcept;
    ParseStatus parse_request_line() noexcept;
    ParseStatus parse_header() noexcept;
    ParseStatus parse_body() noexcept;
    void process_body();
    void process_status_line() noexcept;
    void process_header() noexcept;
    bool parse_request() noexcept;

public:
    bool receive();
    void process();
    bool send();
    void clear() noexcept;
};
} // namespace suzukaze
