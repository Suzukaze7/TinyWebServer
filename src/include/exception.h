#pragma once
#include <exception>
#include <string>

namespace suzukaze {
class SocketException : public std::exception {
    std::string msg_;

public:
    SocketException() noexcept(noexcept(std::string())) = default;
    SocketException(std::string msg) noexcept;
    SocketException(std::string addr, std::string msg);

    const char *what() const noexcept override;
};

class UrlException : public std::exception {
    std::string msg_;

public:
    UrlException() noexcept(noexcept(std::string())) = default;
    UrlException(std::string msg) noexcept;
};

class HandlerException : public std::exception {
    std::string msg_;

public:
    HandlerException() noexcept(noexcept(std::string())) = default;
    HandlerException(std::string msg) noexcept;

    const char *what() const noexcept override;
};

namespace json {
class JsonException : public std::exception {
    std::string msg_;

public:
    JsonException() noexcept(noexcept(std::string())) = default;
    JsonException(std::string msg) noexcept;

    const char *what() const noexcept override;
};
}
} // namespace suzukaze