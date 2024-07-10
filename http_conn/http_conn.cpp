#include "http_conn.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <unistd.h>

namespace suzukaze {
bool HttpConn::receive() {
    int cnt;
    while ((cnt = read(FD, buf, LEN)) > 0)
        msg += buf;
    return errno == EAGAIN;
}

bool HttpConn::parse_request() {
    // todo
    return true;
}

void HttpConn::process() {
    msg = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html; charset=UTF-8\r\n\
\r\n\
<html>\r\n\
    <head></head>\r\n\
    <body>\r\n\
        <h1>hello</h1>\r\n\
    </body>\r\n\
</html>";
}

bool HttpConn::send() {
    write(FD, msg.c_str(), msg.size());
    return true;
}
} // namespace suzukaze