#include "include/http_request.h"
#include "include/exception.h"
#include "include/header.h"
#include "include/json.h"

namespace suzukaze {
auto HttpRequest::get_header(const std::string &key) const noexcept -> const std::string & {
    return info_.headers_.at(key);
}

auto HttpRequest::get_url() const noexcept -> const std::string & { return info_.url_; }

auto HttpRequest::get_scheme() const noexcept -> const std::string & { return info_.scheme_; }

auto HttpRequest::get_body() const noexcept -> const std::string & { return info_.body_; }

auto HttpRequest::get_json() const -> json::Value {
    if (!info_.headers_.contains(CONTENT_TYPE) || info_.headers_.at(CONTENT_TYPE) != JSON_TYPE)
        throw HandlerException("request body isn't json");

    return json::Parser().parse(info_.body_);
}
} // namespace suzukaze