#pragma once
#include <cstring>
#include <exception>
#include <string>

namespace suzukaze {
class SysCallException : public std::exception {
    std::string msg_;

public:
    SysCallException() = default;

    SysCallException(std::string addr) : msg_(std::move((addr += ' ') += strerror(errno))) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

class UrlException : public std::exception {
    std::string msg_;

public:
    UrlException() = default;
    UrlException(std::string msg) noexcept : msg_(std::move(msg)) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

class HandlerException : public std::exception {
    std::string msg_;

public:
    HandlerException() = default;
    HandlerException(std::string msg) noexcept : msg_(std::move(msg)) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};

namespace json {
class JsonException : public std::exception {
    std::string msg_;

public:
    JsonException() = default;
    JsonException(std::string msg) noexcept : msg_(std::move(msg)) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};
} // namespace json

class AllocException : public std::exception {
    std::string msg_;

public:
    AllocException() = default;
    AllocException(std::string msg) noexcept : msg_(std::move(msg)) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};
} // namespace suzukaze