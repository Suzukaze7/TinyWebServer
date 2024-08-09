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
    std::size_t idx_{};
    std::string msg_, line_;
    std::string req_line_;
    RequestMethod method_;
    std::string url_, scheme_;
    std::unordered_map<std::string, std::string> params_;
    std::size_t content_length_{};
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

class HttpRequest {
    const RequestInfo &info_;

public:
    HttpRequest(RequestInfo &info) noexcept : info_(info){};

    const std::string &get_param(const std::string &key) const noexcept;
    const std::string &get_header(const std::string &key) const noexcept;
    const std::string &get_url() const noexcept;
    const std::string &get_scheme() const noexcept;
    const std::string &get_body() const noexcept;
    json::Value get_json() const;
};
} // namespace suzukaze