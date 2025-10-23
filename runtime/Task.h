#pragma once

#include <functional>

struct Task {
    Task(const Task&) noexcept = default;
    Task(Task&&) noexcept = default;

    template <std::invocable F>
    Task(F&& f) : func(std::forward<F>(f)) {}

    std::function<void()> func;
};

