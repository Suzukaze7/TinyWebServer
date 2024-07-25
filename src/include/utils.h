#pragma once
#include <string>
#include <type_traits>

namespace suzukaze {
auto error() noexcept -> std::string;

template <typename Enum>
inline auto to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}
} // namespace suzukaze