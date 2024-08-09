#include "include/http_request.h"
#include "include/exception.h"
#include "include/header.h"
#include "include/json.h"

namespace suzukaze {
const std::string &HttpRequest::get_param(const std::string &key) const noexcept {
    return info_.params_.at(key);
}

const std::string &HttpRequest::get_header(const std::string &key) const noexcept {
    return info_.headers_.at(key);
}

const std::string &HttpRequest::get_url() const noexcept { return info_.url_; }

const std::string &HttpRequest::get_scheme() const noexcept { return info_.scheme_; }

const std::string &HttpRequest::get_body() const noexcept { return info_.body_; }

json::Value HttpRequest::get_json() const {
    if (!info_.headers_.contains(CONTENT_TYPE) || info_.headers_.at(CONTENT_TYPE) != JSON_TYPE)
        throw HandlerException("request body isn't json");

    return json::Parser().parse(info_.body_);
}
} // namespace suzukaze