#pragma once
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "router.h"
#include "type.h"
#include <bits/types/struct_iovec.h>
#include <string>
#include <type_traits>
#include <unistd.h>

namespace suzukaze {
enum class ParseStatus { CONTINUE, NOT_FINISH, FINISH };
enum class SendStatus { NOT_FINISH, CLOSE, KEEP_ALIVE };

class HttpConn {
    static inline Logger logger;
    static inline Router &router = Router::get_instance();

    const fd_t CONN_FD_ = -1;
    RequestInfo req_;
    ResponseInfo resp_;

    auto getline() noexcept -> bool;
    auto parse_request_line() noexcept -> ParseStatus;
    auto parse_header() noexcept -> ParseStatus;
    auto parse_body() noexcept -> ParseStatus;
    void process_body() noexcept;
    void process_status_line() noexcept;
    void process_header() noexcept;

public:
    HttpConn() noexcept = default;
    HttpConn(fd_t fd) noexcept : CONN_FD_(fd) {}

    ~HttpConn() {
        if (resp_.is_file_)
            close(resp_.file_fd_);
    }

    auto receive() -> bool;
    auto parse_request() noexcept -> ParseStatus;
    void process() noexcept;
    auto send() -> SendStatus;
};
} // namespace suzukaze
