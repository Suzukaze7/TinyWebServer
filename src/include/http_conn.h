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
#include <utility>

namespace suzukaze {
enum class ParseStatus { CONTINUE, NOT_FINISH, FINISH };
enum class SendStatus { NOT_FINISH, CLOSE, KEEP_ALIVE };

class HttpConn {
    inline static const std::map<StatusCode, std::pair<std::uint16_t, std::string>> STATUS_INFO = {
        {StatusCode::OK, {200, "OK"}},
        {StatusCode::BAD_REQUEST, {400, "Bad Request"}},
        {StatusCode::NOT_FOUND, {404, "Not Found"}},
        {StatusCode::NOT_ALLOWED, {405, "Method Not Allowed"}},
        {StatusCode::INTERAL_ERROR, {500, "Internal Server Error"}},
    };

    Logger &logger_;
    RootRouter &router_;

    std::string host_;
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
    HttpConn(Logger &logger, RootRouter &router, std::string host) noexcept
        : logger_(logger), router_(router), host_(std::move(host)) {}

    ~HttpConn() {
        if (resp_.is_file_)
            munmap(resp_.file_ptr_, resp_.file_size_);
    }

    auto host() -> std::string &&;
    auto receive(fd_t fd) -> bool;
    auto parse_request() noexcept -> ParseStatus;
    void process() noexcept;
    auto send(fd_t fd) -> SendStatus;
};
} // namespace suzukaze
