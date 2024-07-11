#include <cstring>
#include <string>
#include "utils.h"

namespace suzukaze {
auto error() -> std::string { return strerror(errno); }
} // namespace suzukaze