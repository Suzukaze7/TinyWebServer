#pragma once
#include <exception>
#include <string>

namespace suzukaze {
class SocketException : public std::exception {
    std::string msg_;

public:
    SocketException() noexcept(noexcept(std::string())) = default;
    SocketException(std::string msg) noexcept : msg_(msg) {}
    SocketException(std::string addr, std::string msg) : msg_(addr + " " + msg) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

class UrlException : public std::exception {
    std::string msg_;

public:
    UrlException() noexcept(noexcept(std::string())) = default;
    UrlException(std::string msg) noexcept : msg_(msg) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

class HandlerException : public std::exception {
    std::string msg_;

public:
    HandlerException() noexcept(noexcept(std::string())) = default;
    HandlerException(std::string msg) noexcept : msg_(msg) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

namespace json {
class JsonException : public std::exception {
    std::string msg_;

public:
    JsonException() noexcept(noexcept(std::string())) = default;
    JsonException(std::string msg) noexcept : msg_(msg) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};
} // namespace json
} // namespace suzukaze