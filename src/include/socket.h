#pragma once
#include <cstdint>
#include <string_view>
#include <utility>

namespace suzukaze {
class Descriptor {
    friend class IOMultiplex;

protected:
#if __linux
    using fd_t = int;
#elif _WIN32
#endif

protected:
    fd_t fd_{-1};

public:
    constexpr Descriptor() = default;
    constexpr Descriptor(Descriptor &&oth) noexcept { std::swap(fd_, oth.fd_); }

protected:
    constexpr Descriptor(fd_t fd) noexcept : fd_(fd) {}

public:
    static const Descriptor STDIN;
};

constexpr Descriptor Descriptor::STDIN{1};

class Socket : public Descriptor {
    Socket(fd_t fd) noexcept : Descriptor(fd) {}

public:
    Socket(std::string_view ip, std::uint16_t port);
    Socket(Socket &&oth) noexcept : Descriptor(std::move(oth)) {}

    ~Socket();

private:
    void set_nonblock(fd_t fd) const;

public:
    operator bool() const noexcept { return ~fd_; }

    void listen() const;

    Socket accept() const;
};
} // namespace suzukaze