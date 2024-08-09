#pragma once
#include "container.hpp"
#include "type.h"
#include "utils.h"
#include <bits/types/siginfo_t.h>
#include <bits/types/timer_t.h>
#include <cstddef>
#include <ctime>
#include <functional>
#include <unistd.h>
#include <vector>

namespace suzukaze {

class TimerWheel {
public:
    using Task = std::function<void()>;

private:
    using TaskList = DList<Task>;

public:
    class Pointer {
        friend class TimerWheel;

        TaskList::iterator it_;

        Pointer(TaskList::iterator it) noexcept : it_(it) {}

    public:
        Pointer() = default;
    };

private:
    std::time_t period_;
    int pipe_fd_[2];
    timer_t timer_;
    std::vector<TaskList> wheel_;
    std::size_t idx_;

    static void sig_handler(int sig, siginfo_t *info, void *);

public:
    explicit TimerWheel(std::time_t period = 1, std::size_t wheel_len = 60) noexcept;

    ~TimerWheel();

    fd_t read_fd() noexcept;
    Pointer add_task(Task task);
    void erase_task(Pointer ptr) noexcept;
    void modify_task(Pointer &ptr) noexcept;
    void solve_task() noexcept;
    void start() noexcept;
};
} // namespace suzukaze