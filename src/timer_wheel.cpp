#include "include/timer_wheel.h"
#include "include/descriptor.h"
#include <chrono>
#include <stop_token>
#include <thread>
#include <utility>

namespace suzukaze {
TimerWheel::TimerWheel() {
    auto [read_fd, write_fd] = Descriptor::pipe();
    pipe_fd_ = std::move(read_fd);
    timer_thread_ = std::jthread{[](std::stop_token token, Descriptor fd) {
                                     while (!token.stop_requested()) {
                                         std::this_thread::sleep_for(std::chrono::seconds(1));
                                         fd.write("1");
                                     }
                                 },
                                 std::move(write_fd)};
}

void TimerWheel::solve_task() {
    idx_ = (idx_ + 1) % Period;
    for (auto &task : wheel_[idx_])
        task();
    wheel_[idx_].clear();
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