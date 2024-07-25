#include "include/http_conn.h"
#include "include/exception.h"
#include "include/header.h"
#include "include/http_response.h"
#include "include/logger.hpp"
#include "include/utils.h"
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

namespace suzukaze {
auto HttpConn::receive() -> bool {
    static constexpr std::size_t LEN = 1500;
    static char buf[LEN];

    ssize_t cnt;
    while ((cnt = read(CONN_FD_, buf, LEN)) > 0)
        req_.msg_.append(buf, cnt);

    if (errno != EAGAIN)
        throw SocketException("HttpConn::receive", error());

    return !cnt;
}

auto HttpConn::getline() noexcept -> bool {
    auto &idx = req_.idx_;
    auto &msg = req_.msg_;
    auto &line = req_.line_;

    while (idx < msg.size()) {
        if (msg[idx] == '\r' && idx + 1 < msg.size() && msg[idx + 1] == '\n') {
            idx += 2;
            return true;
        }
        line += msg[idx++];
    }
    return false;
}

auto HttpConn::parse_request_line() noexcept -> ParseStatus {
    std::string method;
    std::istringstream sin(req_.line_);
    sin >> method >> req_.url_ >> req_.scheme_;
    if (!sin) {
        resp_.status_code_ = StatusCode::BAD_REQUEST;
        return ParseStatus::FINISH;
    }

    static const std::map<std::string, RequestMethod> to = {{"GET", RequestMethod::GET},
                                                            {"POST", RequestMethod::POST}};

    req_.method_ = to.at(method);
    req_.keep_alive_ = !(req_.scheme_ == "HTTP/1.0");
    req_.parse_step_ = ParseStep::HEADER;
    return ParseStatus::CONTINUE;
}

auto HttpConn::parse_header() noexcept -> ParseStatus {
    auto &line = req_.line_;

    if (line.empty()) {
        if (!req_.content_length_)
            return ParseStatus::FINISH;
        req_.parse_step_ = ParseStep::BODY;
        return ParseStatus::CONTINUE;
    }

    std::size_t pos = line.find(':');
    if (pos == std::string::npos) {
        resp_.status_code_ = StatusCode::BAD_REQUEST;
        return ParseStatus::FINISH;
    }

    std::string key = line.substr(0, pos),
                value = line.substr(pos + 2); //:后有一个空格

    // todo: need optimize
    if (key == CONTENT_LENGTH) {
        try {
            req_.content_length_ = std::stoull(value);
        } catch (std::exception e) {
            resp_.status_code_ = StatusCode::BAD_REQUEST;
            return ParseStatus::FINISH;
        }
    } else if (key == CONNECTION)
        req_.keep_alive_ = (value == KEEP_ALIVE);

    req_.headers_[key] = std::move(value);
    return ParseStatus::CONTINUE;
}

auto HttpConn::parse_body() noexcept -> ParseStatus {
    req_.body_ += req_.line_;
    if (req_.body_.size() < req_.content_length_)
        return ParseStatus::NOT_FINISH;
    return ParseStatus::FINISH;
}

auto HttpConn::parse_request() noexcept -> ParseStatus {
    auto &parse_step = req_.parse_step_;

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

        req_.line_.clear();

        switch (parse_status) {
        case ParseStatus::CONTINUE:
            break;
        default:
            return parse_status;
        }
    }

    return ParseStatus::NOT_FINISH;
}

void HttpConn::process_status_line() noexcept {
    static const std::map<StatusCode, std::string> status_info = {
        {StatusCode::OK, "OK"},
        {StatusCode::BAD_REQUEST, "Bad Request"},
        {StatusCode::NOT_FOUND, "Not Found"},
        {StatusCode::INTERAL_ERROR, "Internal Server Error"},
    };

    std::format_to(std::back_inserter(resp_.send_header_), "{} {} {}\r\n", req_.scheme_,
                   to_underlying(resp_.status_code_), status_info.at(resp_.status_code_));
}

void HttpConn::process_header() noexcept {
    if (resp_.status_code_ != StatusCode::OK)
        resp_.headers_.clear();

    resp_.headers_.try_emplace(CONNECTION, req_.headers_[CONNECTION]);

    auto &send_header = resp_.send_header_;
    auto it = std::back_inserter(send_header);
    for (auto &[k, v] : resp_.headers_)
        std::format_to(it, "{}: {}\r\n", k, v);
    send_header += "\r\n";

    resp_.vec_[0] = {send_header.data(), send_header.size()};
}

void HttpConn::process_body() noexcept {
    if (resp_.status_code_ == StatusCode::OK) {
        if (resp_.headers_.contains(CONTENT_TYPE)) {
            auto &vec = resp_.vec_;
            if (resp_.is_file_) {
                auto &file_fd = resp_.file_fd_;
                auto &file_size = resp_.file_size_;

                file_fd = open(resp_.body_.c_str(), O_RDONLY);
                file_size = std::filesystem::file_size(resp_.body_);
                void *p;
                if ((p = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0)) ==
                    MAP_FAILED) {
                    resp_.status_code_ = StatusCode::INTERAL_ERROR;
                    return;
                }
                vec[1] = {p, file_size};
                resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(file_size));
            } else {
                vec[1] = {resp_.body_.data(), resp_.body_.size()};
                resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(resp_.body_.size()));
            }
        }
    }
}

void HttpConn::process() noexcept {
    if (resp_.status_code_ == StatusCode::OK) {
        try {
            router.get_handler(req_.method_, req_.url_)(req_, resp_);
        } catch (UrlException e) {
            resp_.status_code_ = StatusCode::NOT_FOUND;
        } catch (std::exception &e) {
            resp_.status_code_ = StatusCode::INTERAL_ERROR;
        }
    }

    process_body();
    process_status_line();
    process_header();
}

SendStatus HttpConn::send() {
    auto &vec = resp_.vec_;

    ssize_t cnt;
    while ((vec[0].iov_len || vec[1].iov_len) &&
           (cnt = writev(CONN_FD_, vec, std::size(vec))) > 0) {
        for (auto &v : vec) {
            auto len = std::min<std::size_t>(v.iov_len, cnt);
            v.iov_base = static_cast<std::uint8_t *>(v.iov_base) + len;
            v.iov_len += len;
            cnt -= len;
        }
    }

    if (!vec[0].iov_len && vec[1].iov_len)
        return SendStatus::NOT_FINISH;
    return req_.keep_alive_ ? SendStatus::KEEP_ALIVE : SendStatus::CLOSE;
}
} // namespace suzukaze