#pragma once
#include "http_request.h"
#include "http_response.h"
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace suzukaze {
using Handler = std::function<void(HttpRequest, HttpResponse)>;

class Router {
    struct RouteNode {
        std::unordered_map<std::string, std::shared_ptr<RouteNode>> next_;
        std::array<Handler, 2> handlers_;
    };

    std::shared_ptr<RouteNode> root_ = std::make_shared<RouteNode>();

    static Router instance;

    Router() noexcept = default;
    Router(Router &) = delete;
    Router(Router &&) = delete;

public:
    static Router &get_instance();

    void add_handler(RequestMethod method, std::string_view url, Handler handler);

    auto get_handler(RequestMethod method, std::string_view url) -> Handler &;
};
} // namespace suzukaze