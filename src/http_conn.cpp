#include "include/http_conn.h"
#include "include/exception.h"
#include "include/logger.hpp"
#include "include/utils.h"
#include <cerrno>
#include <cstddef>
#include <format>
#include <sstream>
#include <string>
#include <unistd.h>

namespace suzukaze {
void HttpConn::receive_msg() {
    ssize_t cnt;
    while ((cnt = read(FD, buf, LEN)) > 0)
        msg += buf;
    if (errno != EAGAIN)
        throw SocketException("HttpConn::receive", error());
}

auto HttpConn::getline() -> bool {
    while (idx < msg.size()) {
        if (idx + 1 < msg.size() && msg.substr(idx, 2) == "\r\n") {
            idx += 2;
            return true;
        }
        line += msg[idx++];
    }
    return false;
}

auto HttpConn::parse_request_line() -> ParseStatus {
    std::istringstream sin(line);
    sin >> method >> url >> ver;
    if (!sin)
        return ParseStatus::BAD_REQUEST;

    keep_alive = !(ver == "HTTP1.0");
    parse_step = ParseStep::HEADER;
    return ParseStatus::OK;
}

auto HttpConn::parse_header() -> ParseStatus {
    if (line.empty()) {
        if (!content_length)
            return ParseStatus::FINISH;
        parse_step = ParseStep::BODY;
        return ParseStatus::OK;
    }

    std::size_t pos = line.find(':');
    if (pos == std::string::npos)
        return ParseStatus::BAD_REQUEST;

    std::string key = line.substr(0, pos), value = line.substr(pos + 2); //:后有一个空格
    if (key == "Content-Length")
        content_length = std::stoull(value);
    else if (key == "Connection")
        keep_alive = (value == "Keep-Alive");
    else
        request[key] = std::move(value);
    return ParseStatus::OK;
}
auto HttpConn::parse_body() -> ParseStatus {
    if (content_length == -1)
        return ParseStatus::BAD_REQUEST;
    body += line;
    if (body.size() < content_length)
        return ParseStatus::NOT_FINISH;
    return ParseStatus::FINISH;
}

ParseStatus HttpConn::parse_request() {
    Logger::debug("parse_request fd {}", FD);

    while (getline() || parse_step == ParseStep::BODY) {
        ParseStatus parse_status;
        switch (parse_step) {
        case ParseStep::REQUEST_LINE:
            parse_status = parse_request_line();
            break;
        case ParseStep::HEADER:
            parse_status = parse_header();
            break;
        case ParseStep::BODY:
            parse_status = parse_body();
            break;
        }

        line.clear();

        switch (parse_status) {
        case ParseStatus::OK:
            break;
        default:
            return parse_status;
        }
    }

    return ParseStatus::NOT_FINISH;
}

void HttpConn::process(ParseStatus parse_status) {
    msg = R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8

)";
    idx = 0;

    if (url == "/") {
        msg += R"(<html>
    <head></head>
    <body>
        <h1>hello</h1>
    </body>
</html>)";
    } else {
        msg += std::format(R"(<html>
    <head></head>
    <body>
        <h1>{}</h1>
    </body>
</html>)",
                           url);
    }
}

SendStatus HttpConn::send_msg() {
    ssize_t cnt;
    while ((cnt = write(FD, msg.c_str() + idx, msg.size() - idx)) > 0)
        idx += cnt;

    if (idx < msg.size())
        return SendStatus::NOT_FINISH;
    return (keep_alive ? SendStatus::KEEP_ALIVE : SendStatus::CLOSE);
}
} // namespace suzukaze