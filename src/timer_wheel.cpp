#include "include/timer_wheel.h"

namespace suzukaze {
void TimerWheel::timer(std::stop_token token, std::binary_semaphore &sema, Result::Handle handle) {
    while (!token.stop_requested()) {
        sema.acquire();
        std::this_thread::sleep_for(std::chrono::seconds(Period));
    }
}

TimerWheel::Result TimerWheel::solve_task() {
    while (true) {
        idx_ = (idx_ + 1) % Period;
        for (auto &task : wheel_[idx_])
            task();
        wheel_[idx_].clear();
    }
}

TimerWheel::Pointer TimerWheel::add_task(Task task) {
    return Pointer{wheel_[idx_].insert(wheel_[idx_].end(), std::move(task))};
}

void TimerWheel::erase_task(Pointer ptr) noexcept {
    auto it = ptr.it_;
    TaskList::erase(it);
}

void TimerWheel::modify_task(Pointer &ptr) noexcept {
    auto it = ptr.it_;
    auto task = std::move(*it);
    TaskList::erase(it);
    ptr = add_task(task);
}
} // namespace suzukaze