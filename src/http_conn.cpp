#include "include/http_conn.h"
#include "include/exception.h"
#include "include/header.h"
#include "include/http_response.h"
#include "include/logger.hpp"
#include "include/type.h"
#include "include/utils.h"
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iterator>
#include <map>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <utility>

namespace suzukaze {
using namespace std::string_view_literals;

auto HttpConn::host() -> std::string && { return std::move(host_); }

auto HttpConn::receive(fd_t fd) -> bool {
    static constexpr std::size_t LEN = 1500;
    static char buf[LEN];

    ssize_t cnt;
    while ((cnt = read(fd, buf, LEN)) > 0)
        req_.msg_.append(buf, cnt);

    if (errno != EAGAIN) {
        logger_.debug("HttpConn::receive error: {}", error());
        return true;
    }

    return !cnt;
}

auto HttpConn::equal_ignore_case(std::string_view lhs, std::string_view rhs) noexcept -> bool {
    return std::ranges::equal(lhs, rhs, std::equal_to<>(), tolower);
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
    req_.req_line_ = std::move(req_.line_);
    auto &line = req_.req_line_;

    static const std::regex PATTERN(R"((\w+)\s+(\S+)\s+(HTTP\/\d+\.\d+))");
    std::smatch res;
    if (!std::regex_match(line, res, PATTERN)) {
        resp_.status_code_ = StatusCode::BAD_REQUEST;
        return ParseStatus::FINISH;
    }

    static const std::map<std::string_view, RequestMethod> to = {{"GET", RequestMethod::GET},
                                                                 {"POST", RequestMethod::POST}};
    try {
        req_.method_ = to.at(std::string_view(res[1].first, res[1].second));
    } catch (...) {
        resp_.status_code_ = StatusCode::NOT_ALLOWED;
        return ParseStatus::FINISH;
    }

    static const std::regex URL_PATTERN(R"([^?]+)"), PARAM_PATTERN(R"(([^?&]+)=([^?&]+))");
    std::smatch url_res;
    if (!std::regex_search(res[2].first, res[2].second, url_res, URL_PATTERN)) {
        resp_.status_code_ = StatusCode::BAD_REQUEST;
        return ParseStatus::FINISH;
    }
    req_.url_ = url_res[0];
    for (std::sregex_iterator it(url_res[0].first, url_res[0].second, PARAM_PATTERN), ed; it != ed;
         it++) {
        req_.params_.emplace((*it)[1], (*it)[2]);
    }
    for (std::regex_iterator<std::string::const_iterator>
             it(res[2].first, res[2].second, URL_PATTERN),
         ed;
         it != ed; it++) {
    }

    req_.scheme_ = res[3];
    keep_alive_ = (req_.scheme_ > "HTTP/1.0"sv); // 加上 sv 就没有 std::string 复制开销？

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

    static const std::regex PATTERN(R"(([^:]+)\s*:\s*(.*))");
    std::smatch res;
    if (!std::regex_match(line.cbegin(), line.cend(), res, PATTERN)) {
        resp_.status_code_ = StatusCode::BAD_REQUEST;
        return ParseStatus::FINISH;
    }

    // todo: need optimize and more strict
    std::string key = res[1], value = res[2];
    if (key == CONTENT_LENGTH) {
        try {
            req_.content_length_ = std::stoull(value);
        } catch (...) {
            resp_.status_code_ = StatusCode::BAD_REQUEST;
            return ParseStatus::FINISH;
        }
    } else if (key == CONNECTION)
        keep_alive_ = equal_ignore_case(value, KEEP_ALIVE);

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
    auto &[code, info] = STATUS_INFO.at(resp_.status_code_);
    std::format_to(std::back_inserter(resp_.send_header_), "{} {} {}\r\n", req_.scheme_, code,
                   info);
}

void HttpConn::process_header() noexcept {
    if (resp_.status_code_ != StatusCode::OK)
        resp_.headers_.clear();

    {
        auto [it, succ] = resp_.headers_.try_emplace(CONNECTION, keep_alive_ ? KEEP_ALIVE : CLOSE);
        if (!succ)
            keep_alive_ = equal_ignore_case(it->second, KEEP_ALIVE);
    }

    auto &send_header = resp_.send_header_;
    auto it = std::back_inserter(send_header);
    for (auto &[k, v] : resp_.headers_)
        std::format_to(it, "{}: {}\r\n", k, v);
    send_header += "\r\n";

    resp_.vec_[0] = {send_header.data(), send_header.size()};
}

void HttpConn::process_body() noexcept {
    if (resp_.status_code_ == StatusCode::OK && !resp_.body_.empty()) {
        auto &vec = resp_.vec_;
        if (resp_.is_file_) {
            auto &file_ptr = resp_.file_ptr_;
            auto &file_size = resp_.file_size_;

            auto file_path = router_.real_file_path(resp_.body_);
            fd_t file_fd;
            if ((file_fd = open(file_path.c_str(), O_RDONLY)) == -1 ||
                (file_size = std::filesystem::file_size(file_path)) ==
                    static_cast<std::uintmax_t>(-1) ||
                (file_ptr = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0)) ==
                    MAP_FAILED) {
                resp_.status_code_ = StatusCode::INTERAL_ERROR;
                return;
            }
            close(file_fd);

            vec[1] = {file_ptr, file_size};
            resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(file_size));
        } else {
            vec[1] = {resp_.body_.data(), resp_.body_.size()};
            resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(resp_.body_.size()));
        }
    }
}

void HttpConn::process() noexcept {
    if (resp_.status_code_ == StatusCode::OK)
        try {
            router_.get_handler(req_.method_, req_.url_)(req_, resp_);
        } catch (UrlException &e) {
            resp_.status_code_ = StatusCode::NOT_FOUND;
        } catch (std::exception &e) {
            resp_.status_code_ = StatusCode::INTERAL_ERROR;
        }

    process_body();
    process_status_line();
    process_header();

    auto &[code, info] = STATUS_INFO.at(resp_.status_code_);
    logger_.info("request: {} {} {} {}", host_, req_.req_line_, code, info);
}

SendStatus HttpConn::send(fd_t fd) {
    auto &vec = resp_.vec_;
    std::size_t idx = 0;

    ssize_t cnt;
    while ((vec[0].iov_len || vec[1].iov_len) && (cnt = writev(fd, vec + idx, 2 - idx)) > 0) {
        for (auto i = idx; cnt; i++) {
            auto len = std::min<std::size_t>(vec[i].iov_len, cnt);
            vec[i].iov_base = static_cast<std::int8_t *>(vec[i].iov_base) + len;
            vec[i].iov_len -= len;
            cnt -= len;

            if (!vec[i].iov_len)
                idx++;
        }
    }

    if (errno != EAGAIN) // SIGPIPE
        return SendStatus::CLOSE;

    if (vec[0].iov_len || vec[1].iov_len)
        return SendStatus::NOT_FINISH;
    return keep_alive_ ? SendStatus::KEEP_ALIVE : SendStatus::CLOSE;
}
} // namespace suzukaze