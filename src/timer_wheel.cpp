#include "include/timer_wheel.h"
#include "include/exception.h"
#include "include/logger.hpp"
#include "include/type.h"
#include "include/utils.h"
#include <bits/types/sigevent_t.h>
#include <bits/types/struct_itimerspec.h>
#include <csignal>
#include <cstddef>
#include <ctime>
#include <unistd.h>
#include <utility>

namespace suzukaze {
TimerWheel::TimerWheel(std::time_t period, std::size_t wheel_len) noexcept
    : period_(period), wheel_(wheel_len), idx_() {
    pipe(pipe_fd_);
    set_nonblock(pipe_fd_[0]), set_nonblock(pipe_fd_[1]);

    struct sigaction act = {.sa_flags = SA_SIGINFO | SA_RESTART};
    act.sa_sigaction = sig_handler;
    sigaction(SIGUSR1, &act, nullptr);

    sigevent ev = {
        .sigev_value = {.sival_int = pipe_fd_[1]},
        .sigev_signo = SIGUSR1,
        .sigev_notify = SIGEV_SIGNAL,
    };
    timer_create(CLOCK_REALTIME, &ev, &timer_);
}

TimerWheel::~TimerWheel() {
    close(pipe_fd_[0]), close(pipe_fd_[1]);

    struct sigaction act {};
    act.sa_handler = SIG_DFL;
    sigaction(SIGUSR1, &act, nullptr);

    timer_delete(timer_);
}

void TimerWheel::sig_handler(int sig, siginfo_t *info, void *) {
    write(info->si_value.sival_int, "1", 1);
}

fd_t TimerWheel::read_fd() noexcept { return pipe_fd_[0]; }

TimerWheel::Pointer TimerWheel::add_task(Task task) {
    return wheel_[idx_].insert(wheel_[idx_].end(), std::move(task));
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

void TimerWheel::solve_task() noexcept {
    constexpr size_t LEN = 100;
    static char buf[LEN];

    while (read(pipe_fd_[0], buf, LEN) > 0)
        ;

    idx_ = idx_ + 1 == wheel_.size() ? 0 : idx_ + 1;
    for (auto &task : wheel_[idx_])
        task();
    wheel_[idx_].clear();
}

void TimerWheel::start() noexcept {
    itimerspec iti = {.it_interval = {.tv_sec = period_}, .it_value = {.tv_sec = period_}};
    timer_settime(timer_, 0, &iti, nullptr);
}
} // namespace suzukaze