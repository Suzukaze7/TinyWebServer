#pragma once
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "memory_pool.hpp"
#include "router.h"
#include "timer_wheel.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>

namespace suzukaze {
enum class ParseStatus { CONTINUE, NOT_FINISH, FINISH };
enum class SendStatus { NOT_FINISH, CLOSE, KEEP_ALIVE };

class HttpConn {
    static inline const std::map<StatusCode, std::pair<std::uint16_t, std::string>> STATUS_INFO = {
        {StatusCode::OK, {200, "OK"}},
        {StatusCode::BAD_REQUEST, {400, "Bad Request"}},
        {StatusCode::NOT_FOUND, {404, "Not Found"}},
        {StatusCode::NOT_ALLOWED, {405, "Method Not Allowed"}},
        {StatusCode::INTERAL_ERROR, {500, "Internal Server Error"}},
    };

    Logger &logger_;
    RootRouter &router_;

public:
    TimerWheel::Pointer task_ptr_;

private:
    const fd_t conn_fd_;
    const std::string host_;
    bool keep_alive_;
    RequestInfo req_;
    ResponseInfo resp_;

    bool equal_ignore_case(std::string_view lhs, std::string_view rhs) noexcept;
    bool getline() noexcept;
    ParseStatus parse_request_line() noexcept;
    ParseStatus parse_header() noexcept;
    ParseStatus parse_body() noexcept;
    void process_body() noexcept;
    void process_status_line() noexcept;
    void process_header() noexcept;

public:
    HttpConn(Logger &logger, RootRouter &router, TimerWheel::Pointer task_ptr, fd_t conn_fd,
             std::string host) noexcept
        : logger_(logger), router_(router), task_ptr_(task_ptr), conn_fd_(conn_fd),
          host_(std::move(host)) {}
    HttpConn(HttpConn &) = delete;

    ~HttpConn() {
        if (resp_.is_file_)
            munmap(resp_.file_ptr_, resp_.file_size_);
    }

    void *operator new(std::size_t) { return MemoryPool<HttpConn>::get_instance().allocate(); }

    void operator delete(void *ptr) noexcept {
        MemoryPool<HttpConn>::get_instance().deallocate(ptr);
    }

    bool receive();
    ParseStatus parse_request() noexcept;
    void process() noexcept;
    SendStatus send() noexcept;
    void clear() noexcept;
};
} // namespace suzukaze
