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
    Level level_;

public:
    inline Logger(Level l = Level::DEBUG) noexcept : level_(l) {}

    template <typename... Args>
    inline void log(Level l, std::format_string<Args...> fmt, Args &&...args) noexcept {
        auto s = std::format(fmt, std::forward<Args>(args)...);
        std::cout << s << std::endl;
    }

    template <typename... Args>
    inline void debug(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::DEBUG, std::move(fmt), std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void info(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::INFO, std::move(fmt), std::forward<Args>(args)...);
    }
};
} // namespace suzukaze