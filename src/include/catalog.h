#pragma once

#include "logger.hpp"
#include "router.h"
#include "thread_pool.h"
#include "timer_wheel.h"

namespace suzukaze {
struct Catalog {
    Logger logger_;
    RootRouter router_;
    TimerWheel timer_wheel_;
    ThreadPool thread_pool_;
};
} // namespace suzukaze