#pragma once
#include "../type/type.h"
#include <map>
#include <string>

namespace suzukaze {
enum class ParseStatus { REQUEST_LINE, HEADER, BODY };
enum class LineStatus { OK, NOT_FINISH, FINISH, BAD_REQUEST, INTERAL_ERROR };
enum class StatusCode { NOT_FINISH, OK, BAD_REQUEST, INTERAL_ERROR };

class HttpConn {
    static constexpr std::size_t LEN = 1500;
    static inline char buf[LEN];

    const fd_t FD = -1;
    std::size_t parse_idx = 0;
    ParseStatus parse_status = ParseStatus::REQUEST_LINE;
    std::string msg, line;
    std::string method, url, ver;
    std::size_t content_length = -1;
    std::string body;
    std::map<std::string, std::string> request;

    auto getline() -> bool;
    auto parse_request_line() -> LineStatus;
    auto parse_header() -> LineStatus;
    auto parse_body() -> LineStatus;

public:
    HttpConn() = default;
    inline HttpConn(fd_t fd) : FD(fd){};

    void receive_msg();
    auto parse_request() -> StatusCode;
    void process();
    bool send_msg();
};
} // namespace suzukaze
