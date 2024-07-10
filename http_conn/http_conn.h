#pragma once

#include "../type/type.h"
#include <string>

namespace suzukaze {
enum class StatusCode {

};

class HttpConn {
    static constexpr std::size_t LEN = 1500;
    static inline char buf[LEN];

    const fd_t FD;
    std::string msg;

public:
    inline HttpConn(fd_t fd) : FD(fd){};

    bool receive();
    bool parse_request();
    void process();
    bool send();
};
} // namespace suzukaze
