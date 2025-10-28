#pragma once

#include <functional>

struct Task {
    Task() noexcept = default;
    Task(Task&&) noexcept = default;
    Task& operator=(Task&&) noexcept = default;

    Task(const Task& t) : func(t.func) {
        std::cout << "Copied Task" << std::endl;
    }
    Task& operator=(const Task&) = default;

    template <std::invocable F>
    Task(F&& f) : func(std::forward<F>(f)) {}

    std::function<void()> func;
};
