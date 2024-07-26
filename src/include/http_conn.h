#pragma once
#include "http_request.h"
#include "http_response.h"
#include "logger.hpp"
#include "router.h"
#include "type.h"
#include <bits/types/struct_iovec.h>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <type_traits>
#include <unistd.h>

namespace suzukaze {
enum class ParseStatus { CONTINUE, NOT_FINISH, FINISH };
enum class SendStatus { NOT_FINISH, CLOSE, KEEP_ALIVE };

class HttpConn {
    static inline Logger logger_;
    static inline Router &router_ = Router::get_instance();

    const std::string host_;
    const fd_t fd_ = -1;
    bool keep_alive_;
    RequestInfo req_;
    ResponseInfo resp_;

    auto equal_ignore_case(std::string_view lhs, std::string_view rhs) noexcept -> bool;
    auto getline() noexcept -> bool;
    auto parse_request_line() noexcept -> ParseStatus;
    auto parse_header() noexcept -> ParseStatus;
    auto parse_body() noexcept -> ParseStatus;
    void process_body() noexcept;
    void process_status_line() noexcept;
    void process_header() noexcept;

public:
    HttpConn() noexcept = default;
    HttpConn(fd_t fd) noexcept : fd_(fd) {}

    ~HttpConn();

    auto receive() -> bool;
    auto parse_request() noexcept -> ParseStatus;
    void process() noexcept;
    auto send() -> SendStatus;
};
} // namespace suzukaze
