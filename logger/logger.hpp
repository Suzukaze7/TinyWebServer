#include <format>
#include <iostream>
#include <string_view>
#include <utility>

namespace suzukaze {
enum class Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {

private:
    Level level;

public:
    inline Logger(Level l = Level::DEBUG) : level(l) {}

    template <typename... Args>
    inline void constexpr log(Level l, std::string_view fmt, Args &&...args) {
        auto s = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        std::cout << s << std::endl;
    }

    template <typename... Args>
    inline void constexpr debug(std::string_view fmt, Args &&...args) {
        log(Level::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void constexpr info(std::string_view fmt, Args &&...args) {
        log(Level::INFO, fmt, std::forward<Args>(args)...);
    }
};
} // namespace suzukaze