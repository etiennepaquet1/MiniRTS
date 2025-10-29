#pragma once
#include <utility>
#include <type_traits>
#include <cassert>

struct Task {
    using InvokeFn  = void(*)(void*) noexcept;
    using DestroyFn = void(*)(void*) noexcept;

    void*       callable_ptr = nullptr;
    InvokeFn    invoke_fn    = nullptr;
    DestroyFn   destroy_fn   = nullptr;

    // ───────────────────────────────
    // Default operations (trivial)
    // ───────────────────────────────
    Task() noexcept = default;

    Task(const Task& other) = delete;
    Task& operator=(const Task&) noexcept = default;


    Task(Task&& other) noexcept = default;


    Task& operator=(Task&&) noexcept = default;

    // ───────────────────────────────
    // Construct from callable
    // ───────────────────────────────
    template <typename F>
    requires (!std::same_as<std::decay_t<F>, Task>)
    Task(F&& f) noexcept {
        using Fn = std::decay_t<F>;
        callable_ptr = new Fn(std::forward<F>(f));
        invoke_fn = [](void* p) noexcept { (*static_cast<Fn*>(p))(); };
        destroy_fn = [](void* p) noexcept { delete static_cast<Fn*>(p); };
    }

    // ───────────────────────────────
    // Invoke and manual destroy
    // ───────────────────────────────
    void operator()() const noexcept {
        assert(invoke_fn && callable_ptr);
        invoke_fn(callable_ptr);
    }

    void destroy() noexcept {
        if (destroy_fn && callable_ptr)
            destroy_fn(callable_ptr);
        callable_ptr = nullptr;
        invoke_fn = nullptr;
        destroy_fn = nullptr;
    }
    operator bool() const noexcept {
        return (callable_ptr && invoke_fn && destroy_fn);
    }
};
