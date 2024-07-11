#include "http_conn.h"
#include "../exception/exception.h"
#include "../utils/utils.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <format>
#include <sstream>
#include <string>
#include <unistd.h>

namespace suzukaze {
void HttpConn::receive_msg() {
    int cnt;
    while ((cnt = read(FD, buf, LEN)) > 0)
        msg += buf;
    if (errno != EAGAIN)
        throw SocketException("HttpConn::receive", error());
}

auto HttpConn::getline() -> bool {
    while (parse_idx < msg.size()) {
        if (parse_idx + 1 < msg.size() && msg.substr(parse_idx, 2) == "\r\n") {
            parse_idx += 2;
            return true;
        }
        line += msg[parse_idx++];
    }
    return false;
}

auto HttpConn::parse_request_line() -> LineStatus {
    std::istringstream sin(line);
    sin >> method >> url >> ver;
    if (!sin)
        return LineStatus::BAD_REQUEST;
    
    parse_status = ParseStatus::HEADER;
    return LineStatus::OK;
}

auto HttpConn::parse_header() -> LineStatus {
    if (line.empty()) {
        if (method == "GET")
            return LineStatus::FINISH;
        parse_status = ParseStatus::BODY;
        return LineStatus::OK;
    }

    std::size_t pos = line.find(':');
    if (pos == std::string::npos)
        return LineStatus::BAD_REQUEST;

    std::string k = line.substr(0, pos), v = line.substr(pos + 2); //:后有一个空格
    if (k == "Content-Length")
        content_length = std::stoull(v);
    else
        request[k] = std::move(v);
    return LineStatus::OK;
}
auto HttpConn::parse_body() -> LineStatus {
    if (content_length == -1)
        return LineStatus::BAD_REQUEST;
    body += line;
    if (body.size() < content_length)
        return LineStatus::NOT_FINISH;
    return LineStatus::FINISH;
}

StatusCode HttpConn::parse_request() {
    while (getline() || parse_status == ParseStatus::BODY) {
        LineStatus line_status;
        switch (parse_status) {
        case ParseStatus::REQUEST_LINE:
            line_status = parse_request_line();
            break;
        case ParseStatus::HEADER:
            line_status = parse_header();
            break;
        case ParseStatus::BODY:
            line_status = parse_body();
            break;
        }

        line.clear();

        switch (line_status) {
        case LineStatus::OK:
            break;
        case LineStatus::FINISH:
            return StatusCode::OK;
        case LineStatus::NOT_FINISH:
            return StatusCode::NOT_FINISH;
        case LineStatus::BAD_REQUEST:
            return StatusCode::BAD_REQUEST;
        case LineStatus::INTERAL_ERROR:
            return StatusCode::INTERAL_ERROR;
        }
    }

    return StatusCode::NOT_FINISH;
}

void HttpConn::process() {
    msg = R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8

)";
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
</html>)", url);
    }
}

bool HttpConn::send_msg() {
    write(FD, msg.c_str(), msg.size());
    return true;
}
} // namespace suzukaze