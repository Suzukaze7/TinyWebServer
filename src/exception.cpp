#include "include/exception.h"
#include <string>

namespace suzukaze {

SocketException::SocketException(std::string msg) noexcept : msg(std::move(msg)) {}

SocketException::SocketException(std::string addr, std::string msg) : msg(addr + " " + msg) {}

const char *SocketException::what() const noexcept { return msg.c_str(); }
} // namespace suzukaze