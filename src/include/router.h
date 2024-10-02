#pragma once
#include "http_request.h"
#include "http_response.h"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace suzukaze {
using Handler = std::function<void(HttpRequest, HttpResponse)>;

class Router {
    struct RouteNode {
        std::unordered_map<std::string, std::shared_ptr<RouteNode>> next_;
        std::array<Handler, 2> handlers_;
    };

    std::shared_ptr<RouteNode> root_{std::make_shared<RouteNode>()};

public:
    // Router(Router &&);

    void add_handler(RequestMethod method, std::string_view url, Handler handler);

    Handler &get_handler(RequestMethod method, std::string_view url) const;
};

class RootRouter : public Router {
    struct FileHandler {
        void operator()(HttpRequest req, HttpResponse resp) { resp.file(req.get_url().substr(1)); }
    };
    Handler file_handler_{FileHandler{}};

    const std::string static_dir_;

public:
    explicit RootRouter(std::string &&static_dir) noexcept : static_dir_(std::move(static_dir)) {}

    const Handler &get_handler(RequestMethod method, std::string_view url) const;

    std::string real_file_path(std::string_view url) const noexcept;
};
} // namespace suzukaze