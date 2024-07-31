#include "include/http_response.h"
#include "include/exception.h"
#include "include/header.h"
#include <filesystem>
#include <string_view>
#include <utility>

namespace suzukaze {
void HttpResponse::set_header(std::string key, std::string value) noexcept {
    auto &headers = info_.headers_;
    headers.insert_or_assign(std::move(key), std::move(value));
}

void HttpResponse::plain(std::string data) noexcept {
    info_.headers_[CONTENT_TYPE] = "text/plain; charset=utf-8";
    info_.body_ = std::move(data);
    info_.is_file_ = false;
}

void HttpResponse::file(std::string file_path) {
    if (file_path.starts_with('/') || file_path.ends_with('/'))
        throw HandlerException("file_path should not start or end with /");
    info_.body_ = std::move(file_path);
    info_.is_file_ = true;
}

void HttpResponse::html(std::string file_path) {
    file(std::move(file_path));
    info_.headers_[CONTENT_TYPE] = "text/html; charset=utf-8";
}

void HttpResponse::json(const json::Value &val) {
    info_.body_ = json::Serializer().serialize(val);
    info_.headers_[CONTENT_TYPE] = "application/json; charset=utf-8";
    info_.is_file_ = false;
}
} // namespace suzukaze