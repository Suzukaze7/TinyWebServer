#pragma once

#include "iomultiplex.h"
#include "logger.hpp"
#include "thread_pool.h"
#include "timer_wheel.h"

namespace suzukaze {
struct Tool {
    std::size_t cur_conn_{};
    Logger logger_;
    TimerWheel timer_wheel_;
    ThreadPool thread_pool_;
    IOMultiplex iomultiplex_;
};
} // namespace suzukaze