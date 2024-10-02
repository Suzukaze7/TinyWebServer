#pragma once

#include "descriptor.h"
#include "json.h"
#include <string>
#include <unordered_map>

namespace suzukaze {
enum class StatusCode { OK, BAD_REQUEST, NOT_FOUND, NOT_ALLOWED, INTERNAL_ERROR };

struct ResponseInfo {
    StatusCode status_code_ = StatusCode::OK;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    std::string send_header_;
    bool is_file_{};
    MMap mmap_;
    IOVec<2> vec_;
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