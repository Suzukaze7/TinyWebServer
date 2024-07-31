#pragma once

#include "json.h"
#include "type.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace suzukaze {
enum class StatusCode { OK, BAD_REQUEST, NOT_FOUND, NOT_ALLOWED, INTERAL_ERROR };

struct ResponseInfo {
    StatusCode status_code_ = StatusCode::OK;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    std::string send_header_;
    bool is_file_ = false;
    void *file_ptr_;
    std::size_t file_size_;
    iovec vec_[2]{};
};

class HttpResponse {
    ResponseInfo &info_;

public:
    HttpResponse(ResponseInfo &info) noexcept : info_(info) {}

    void set_header(std::string key, std::string value) noexcept;
    void plain(std::string data) noexcept;
    void file(std::string file_path);
    void html(std::string file_path);
    void json(const json::Value &val);
};
} // namespace suzukaze