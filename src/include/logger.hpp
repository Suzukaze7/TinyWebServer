#pragma once
#include <algorithm>
#include <format>
#include <iostream>
#include <utility>

namespace suzukaze {
enum class Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
    template <typename... Args>
    constexpr void log(Level l, std::format_string<Args...> fmt, Args &&...args) noexcept {
#ifdef NDEBUG
        if constexpr (l == Level::DEBUG)
            return;
#endif
        auto s = std::format(fmt, std::forward<Args>(args)...);
        std::cout << s << std::endl;
    }

    template <typename... Args>
    constexpr void debug(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::DEBUG, std::move(fmt), std::forward<Args>(args)...);
    }

    template <typename... Args>
    constexpr void info(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::INFO, std::move(fmt), std::forward<Args>(args)...);
    }
};
} // namespace suzukaze