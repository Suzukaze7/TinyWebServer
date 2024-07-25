#include "include/router.h"
#include "include/exception.h"
#include "include/utils.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace suzukaze {
Router Router::instance;

Router &Router::get_instance() { return instance; }

void Router::add_handler(RequestMethod method, std::string_view url, Handler handler) {
    if (url.empty() || (url != "/" && (!url.starts_with('/') || url.ends_with('/'))))
        throw UrlException("url should start with '/' and not end with '/'");

    url.remove_prefix(1);
    std::filesystem::path path = url;
    auto type_idx = to_underlying(method);
    auto ptr = root_;
    for (auto &step : path)
        ptr = ptr->next_.try_emplace(step, std::make_shared<RouteNode>()).first->second;

    if (ptr->handlers_[type_idx])
        throw UrlException(std::format("url {} already in use", url));

    ptr->handlers_[type_idx] = std::move(handler);
}

auto Router::get_handler(RequestMethod method, std::string_view url) -> Handler & {
    if (url.empty() || (url != "/" && (!url.starts_with('/') || url.ends_with('/'))))
        throw UrlException("url should start with '/' and not end with '/'");

    url.remove_prefix(1);
    std::filesystem::path path = url;
    auto type_idx = to_underlying(method);
    auto ptr = root_;
    for (auto &step : path) {
        if (!ptr->next_.contains(step))
            throw UrlException("url not exists");
        ptr = ptr->next_[step];
    }

    if (!ptr->handlers_[type_idx])
        throw UrlException("url not exists");

    return ptr->handlers_[type_idx];
}

} // namespace suzukaze