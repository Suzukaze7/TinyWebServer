#pragma once
#include "json.h"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace suzukaze {
enum class RequestMethod { GET, POST };
enum class ParseStep { REQUEST_LINE, HEADER, BODY };

struct RequestInfo {
    ParseStep parse_step_ = ParseStep::REQUEST_LINE;
    std::size_t idx_ = 0;
    std::string msg_, line_;
    std::string req_line_;
    RequestMethod method_;
    std::string url_, scheme_;
    std::unordered_map<std::string, std::string> params_;
    std::size_t content_length_ = 0;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

class HttpRequest {
    const RequestInfo &info_;

public:
    HttpRequest(RequestInfo &info) noexcept : info_(info){};

    auto get_param(const std::string &key) const noexcept -> const std::string &;
    auto get_header(const std::string &key) const noexcept -> const std::string &;
    auto get_url() const noexcept -> const std::string &;
    auto get_scheme() const noexcept -> const std::string &;
    auto get_body() const noexcept -> const std::string &;
    auto get_json() const -> json::Value;
};
} // namespace suzukaze