#pragma once

#include "type.h"
#include <bits/types/struct_iovec.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include "json.h"

namespace suzukaze {
enum class StatusCode { OK = 200, BAD_REQUEST = 400, NOT_FOUND = 404, INTERAL_ERROR = 500 };

struct ResponseInfo {
    StatusCode status_code_ = StatusCode::OK;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    std::string send_header_;
    bool is_file_ = false;
    fd_t file_fd_;
    std::size_t file_size_;
    iovec vec_[2]{};
};

class HttpResponse {
    ResponseInfo &info_;

public:
    HttpResponse(ResponseInfo &info) noexcept : info_(info) {}

    void set_header(std::string key, std::string value);
    void plain(std::string data);
    void html(std::string file_path);
    void json(const json::Value &val);
};
} // namespace suzukaze