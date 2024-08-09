#pragma once
#include "utils.h"
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
#ifdef NDEBUG
    Logger(Level l = Level::INFO) noexcept : level_(l) {}
#else
    Logger(Level l = Level::DEBUG) noexcept : level_(l) {}
#endif

    template <typename... Args>
    static void log(std::format_string<Args...> fmt, Args &&...args) noexcept {
        auto s = std::format(fmt, std::forward<Args>(args)...);
        std::cout << s << std::endl;
    }

    template <typename... Args>
    void log(Level l, std::format_string<Args...> fmt, Args &&...args) noexcept {
        if (to_underlying(l) >= to_underlying(level_)) {
            auto s = std::format(fmt, std::forward<Args>(args)...);
            std::cout << s << std::endl;
        }
    }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::DEBUG, std::move(fmt), std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args &&...args) noexcept {
        log(Level::INFO, std::move(fmt), std::forward<Args>(args)...);
    }
};
} // namespace suzukaze