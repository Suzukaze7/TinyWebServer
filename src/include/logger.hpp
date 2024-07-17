#pragma once
#include <algorithm>
#include <format>
#include <iostream>
#include <string_view>
#include <utility>

namespace suzukaze {
enum class Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {

private:
    inline static Level level = Level::DEBUG;

public:
    inline static void config(Level l) { level = l; }

    template <typename... Args>
    inline static void log(Level l, std::format_string<Args...> fmt, Args &&...args) {
        auto s = std::format(fmt, std::forward<Args>(args)...);
        std::cout << s << std::endl;
    }

    template <typename... Args>
    inline static void debug(std::format_string<Args...> fmt, Args &&...args) {
        log(Level::DEBUG, std::move(fmt), std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void info(std::format_string<Args...> fmt, Args &&...args) {
        log(Level::INFO, std::move(fmt), std::forward<Args>(args)...);
    }
};
} // namespace suzukaze