#include <cstring>
#include <string>
#include "include/utils.h"

namespace suzukaze {
auto error() noexcept -> std::string { return strerror(errno); }
} // namespace suzukaze