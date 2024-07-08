#include <exception>
#include <string>

namespace suzukaze {
class SocketException : public std::exception {
    std::string msg;

public:
    SocketException() = default;
    SocketException(std::string msg) noexcept;
    SocketException(std::string addr, std::string msg);

    const char *what() const noexcept override;
};
} // namespace suzukaze