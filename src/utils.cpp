#include "include/utils.h"
#include <fcntl.h>

namespace suzukaze {
void set_nonblock(fd_t fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
} // namespace suzukaze