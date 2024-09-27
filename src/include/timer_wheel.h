#pragma once
#include "container.hpp"
#include <array>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <semaphore>
#include <stop_token>
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
    struct Result {
        struct Promise;

        using promise_type = Promise;
        using Handle = std::coroutine_handle<promise_type>;

        struct Promise {
            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            Result get_return_object() {
                return {std::coroutine_handle<Promise>::from_promise(*this)};
            }
            void unhandled_exception() {}
        };

        Handle handle_;
    };

private:
    static constexpr std::size_t Period = 60;

    std::array<TaskList, Period> wheel_;
    std::uint8_t idx_;
    std::jthread timer_thread_;
    std::binary_semaphore sema_;

private:
    static void timer(std::stop_token token, std::binary_semaphore &sema, Result::Handle handle);

    Result solve_task();

public:
    TimerWheel();

    Pointer add_task(Task task);
    void erase_task(Pointer ptr) noexcept;
    void modify_task(Pointer &ptr) noexcept;
};
} // namespace suzukaze