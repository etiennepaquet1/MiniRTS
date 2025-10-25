#pragma once

#include <functional>

struct Task {
    Task() noexcept = default;
    Task(Task&&) noexcept = default;
    Task& operator=(Task&&) noexcept = default;

    // Task is move-only
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    template <std::invocable F>
    Task(F&& f) : func(std::forward<F>(f)) {}

    std::move_only_function<void()> func;
};

