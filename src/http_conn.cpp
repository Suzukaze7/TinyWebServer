#include "include/http_conn.h"
#include "include/descriptor.h"
#include "include/exception.h"
#include "include/header.h"
#include "include/http_response.h"
#include <exception>
#include <format>
#include <iterator>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

namespace suzukaze {
using namespace std::string_view_literals;

bool HttpConn::receive() {
    if (!fd_.read(req_.msg_))
        return false;

    tool_->thread_pool_.submit([this, &iomultiplex = tool_->iomultiplex_] {
        if (parse_request()) {
            process();
            iomultiplex.mod(fd_, false);
        } else
            iomultiplex.mod(fd_, true);
    });
    return true;
}

bool HttpConn::equal_ignore_case(std::string_view lhs, std::string_view rhs) noexcept {
    return std::ranges::equal(lhs, rhs, std::equal_to<>(), tolower);
}

bool HttpConn::getline() noexcept {
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

HttpConn::ParseStatus HttpConn::parse_request_line() noexcept {
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
    keep_alive_ = (req_.scheme_ > "HTTP/1.0"sv); // 加上 sv 就没有 std::string 构造开销？

    req_.parse_step_ = ParseStep::HEADER;
    return ParseStatus::CONTINUE;
}

HttpConn::ParseStatus HttpConn::parse_header() noexcept {
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

HttpConn::ParseStatus HttpConn::parse_body() noexcept {
    req_.body_ += req_.line_;
    if (req_.body_.size() < req_.content_length_)
        return ParseStatus::NOT_FINISH;
    return ParseStatus::FINISH;
}

bool HttpConn::parse_request() noexcept {
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
            return parse_status == ParseStatus::FINISH;
        }
    }

    return false;
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

    resp_.vec_.set(0, {send_header.data(), send_header.size()});
}

void HttpConn::process_body() {
    if (resp_.status_code_ == StatusCode::OK && !resp_.body_.empty()) {
        auto &vec = resp_.vec_;
        if (resp_.is_file_) {
            auto file_path = config_->router_.real_file_path(resp_.body_);
            resp_.mmap_ = MMap(file_path);
            vec.set(1, resp_.mmap_);
            resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(resp_.mmap_.size()));
        } else {
            vec.set(1, {resp_.body_.data(), resp_.body_.size()});
            resp_.headers_.insert_or_assign(CONTENT_LENGTH, std::to_string(resp_.body_.size()));
        }
    }
}

void HttpConn::process() {
    if (resp_.status_code_ == StatusCode::OK)
        config_->router_.get_handler(req_.method_, req_.url_)(req_, resp_);
    try {
    } catch (UrlException &e) {
        resp_.status_code_ = StatusCode::NOT_FOUND;
        tool_->logger_.info("not found: {}", e.what());
    } catch (std::exception &e) {
        resp_.status_code_ = StatusCode::INTERNAL_ERROR;
        tool_->logger_.info("interal server error: {}", e.what());
    }

    process_body();
    try {
    } catch (std::exception &e) {
        resp_.status_code_ = StatusCode::INTERNAL_ERROR;
        tool_->logger_.info("interal server error: {}", e.what());
    }

    process_status_line();
    process_header();

    auto &[code, info] = STATUS_INFO.at(resp_.status_code_);
    tool_->logger_.info("request: {} {} {} {}", host_, req_.req_line_, code, info);
}

bool HttpConn::send() {
    if (fd_.write(resp_.vec_)) {
        if (keep_alive_) {
            clear();
            tool_->iomultiplex_.mod(fd_, true);
        }
        return keep_alive_;
    }

    tool_->iomultiplex_.mod(fd_, false);
    return true;
}

void HttpConn::clear() noexcept { req_ = {}, resp_ = {}; }
} // namespace suzukaze