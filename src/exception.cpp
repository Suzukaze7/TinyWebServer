#include "include/exception.h"
#include <string>

namespace suzukaze {

SocketException::SocketException(std::string msg) noexcept : msg(std::move(msg)) {}

SocketException::SocketException(std::string addr, std::string msg) : msg(addr + " " + msg) {}

const char *SocketException::what() const noexcept { return msg.c_str(); }

UrlException::UrlException(std::string msg) noexcept : msg(msg) {}

HandlerException::HandlerException(std::string msg) noexcept : msg(msg) {}

const char *HandlerException::what() const noexcept { return msg.c_str(); }

namespace json {
JsonException::JsonException(std::string msg) noexcept : msg(msg) {}

const char *JsonException::what() const noexcept { return msg.c_str(); }
} // namespace json
} // namespace suzukaze