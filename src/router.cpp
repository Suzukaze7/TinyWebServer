#include "include/router.h"
#include "include/exception.h"
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace suzukaze {
void Router::add_handler(RequestMethod method, std::string_view url, Handler handler) {
    if (url.empty() || (url != "/" && (!url.starts_with('/') || url.ends_with('/'))))
        throw UrlException("url should start with '/' and not end with '/'");

    std::filesystem::path path(url.substr(1));
    auto type_idx = static_cast<std::underlying_type_t<decltype(method)>>(method);
    auto ptr = root_;
    for (auto &step : path)
        ptr = ptr->next_.try_emplace(step, std::make_shared<RouteNode>()).first->second;

    if (ptr->handlers_[type_idx])
        throw UrlException(std::format("url {} already in use", url));

    ptr->handlers_[type_idx] = std::move(handler);
}

Handler &Router::get_handler(RequestMethod method, std::string_view url) const {
    std::filesystem::path path(url.substr(1));
    auto type_idx = static_cast<std::underlying_type_t<decltype(method)>>(method);
    auto ptr = root_;
    for (auto &step : path) {
        if (!ptr->next_.contains(step)) {
            ptr.reset();
            break;
        }
        ptr = ptr->next_[step];
    }

    if (!ptr || !ptr->handlers_[type_idx])
        throw UrlException(std::format("url {} not exists", url));

    return ptr->handlers_[type_idx];
}

const Handler &RootRouter::get_handler(RequestMethod method, std::string_view url) const {
    try {
        return Router::get_handler(method, url);
    } catch (UrlException &e) {
        if (std::filesystem::exists(real_file_path(std::string_view(url.begin() + 1, url.end()))))
            return file_handler_;
        throw;
    }
}

std::string RootRouter::real_file_path(std::string_view url) const noexcept {
    return std::string(static_dir_) += url;
}

} // namespace suzukaze