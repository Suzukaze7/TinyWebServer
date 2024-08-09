#pragma once
#include "exception.h"
#include "http_request.h"
#include "http_response.h"
#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace suzukaze {
using Handler = std::function<void(HttpRequest, HttpResponse)>;

class Router {
    struct RouteNode {
        std::unordered_map<std::string, std::shared_ptr<RouteNode>> next_;
        std::array<Handler, 2> handlers_;
    };

    std::shared_ptr<RouteNode> root_ = std::make_shared<RouteNode>();

public:
    // Router(Router &&);

    void add_handler(RequestMethod method, std::string_view url, Handler handler);

    Handler &get_handler(RequestMethod method, std::string_view url);
};

class RootRouter : public Router {
    struct FileHandler {
        void operator()(HttpRequest req, HttpResponse resp) { resp.file(req.get_url().substr(1)); }
    };
    Handler file_handler_{FileHandler{}};

    std::filesystem::path static_dir_;

public:
    explicit RootRouter(std::string static_dir) noexcept : static_dir_(std::move(static_dir)) {}

    Handler &get_handler(RequestMethod method, std::string_view url);

    std::filesystem::path real_file_path(std::string_view url) noexcept;
};
} // namespace suzukaze