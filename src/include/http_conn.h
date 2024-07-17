#pragma once
#include "type.h"
#include <map>
#include <string>

namespace suzukaze {
enum class ParseStep { REQUEST_LINE, HEADER, BODY };
enum class ParseStatus { NOT_FINISH, FINISH, OK, BAD_REQUEST, INTERAL_ERROR };
enum class SendStatus { NOT_FINISH, CLOSE, KEEP_ALIVE };

class HttpConn {
    static constexpr std::size_t LEN = 1500;
    static inline char buf[LEN];

    const fd_t FD = -1;
    std::size_t idx = 0;
    ParseStep parse_step = ParseStep::REQUEST_LINE;
    std::string msg, line;
    std::string method, url, ver;
    bool keep_alive;
    std::size_t content_length = 0;
    std::string body;
    std::map<std::string, std::string> request;

    auto getline() -> bool;
    auto parse_request_line() -> ParseStatus;
    auto parse_header() -> ParseStatus;
    auto parse_body() -> ParseStatus;

public:
    HttpConn() = default;
    inline HttpConn(fd_t fd) : FD(fd){};

    void receive_msg();
    auto parse_request() -> ParseStatus;
    void process(ParseStatus http_code);
    auto send_msg() -> SendStatus;
};
} // namespace suzukaze
