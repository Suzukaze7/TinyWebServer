#include "include/http_response.h"
#include "include/exception.h"
#include "include/header.h"
#include <filesystem>
#include <string_view>
#include <utility>

namespace suzukaze {
void HttpResponse::set_header(std::string key, std::string value) {
    auto &headers = info_.headers_;
    headers.insert_or_assign(std::move(key), std::move(value));
}

void HttpResponse::plain(std::string data) {
    info_.headers_[CONTENT_TYPE] = "text/plain; charset=utf-8";
    info_.body_ = std::move(data);
    info_.is_file_ = false;
}

void HttpResponse::html(std::string file_path) {
    if (file_path.starts_with('/'))
        throw HandlerException("file path should not start with /");

    file_path = "static/" + file_path;
    if (!std::filesystem::is_regular_file(file_path))
        throw HandlerException("file " + file_path + " not exists or is directory");

    info_.headers_[CONTENT_TYPE] = "text/html; charset=utf-8";
    info_.body_ = std::move(file_path);
    info_.is_file_ = true;
}

void HttpResponse::json(const json::Value &val) {
    info_.headers_[CONTENT_TYPE] = "application/json; charset=utf-8";
    info_.body_ = json::Serializer().serialize(val);
    info_.is_file_ = false;
}
} // namespace suzukaze