#pragma once
#include "type.h"
#include <string>
#include <type_traits>

namespace suzukaze {
template <typename Enum>
inline auto to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

void set_nonblock(fd_t fd);
} // namespace suzukaze