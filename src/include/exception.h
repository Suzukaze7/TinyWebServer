#pragma once
#include <cstring>
#include <exception>
#include <string>
#include <utility>

namespace suzukaze {
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

class AllocException : public std::exception {
    std::string msg_;

public:
    AllocException() noexcept(noexcept(std::string())) = default;
    AllocException(std::string msg) noexcept : msg_(msg) {}

    const char *what() const noexcept override { return msg_.c_str(); }
};
} // namespace suzukaze