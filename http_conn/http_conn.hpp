#pragma once
#include <cerrno>
#include <netdb.h>
#include <sys/socket.h>

namespace suzukaze {

using fd_t = int;
using err_t = int;

class HttpConn {
    static inline fd_t listen_fd;
    static void start_listen();
};

inline void HttpConn::start_listen() {
    addrinfo hint{}, *result;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
    err_t err;
    if ((err = getaddrinfo(nullptr, "http", &hint, &result)))
        throw gai_strerror(err);
    
    for (addrinfo *p = result; p; p = p->ai_next)
        debug(*p);
}
} // namespace suzukaze
