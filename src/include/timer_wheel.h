#pragma once
#include "container.hpp"
#include "descriptor.h"
#include <array>
#include <functional>
#include <thread>

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

        explicit Pointer(TaskList::iterator it) noexcept : it_(it) {}

    public:
        Pointer() = default;
    };

private:
    static constexpr std::size_t Period = 60;

    std::array<TaskList, Period> wheel_;
    std::uint8_t idx_{};
    Descriptor pipe_fd_;
    std::jthread timer_thread_;

public:
    TimerWheel();

    const Descriptor &read_fd() noexcept { return pipe_fd_; }
    void solve_task();
    Pointer add_task(Task task);
    void erase_task(Pointer ptr) noexcept;
    void modify_task(Pointer &ptr) noexcept;
};
} // namespace suzukaze