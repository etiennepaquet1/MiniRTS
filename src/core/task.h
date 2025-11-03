/**
 * @file task.h
 * @brief Defines the Task type — a small, type-erased callable wrapper
 *        used by the RTS scheduler to represent units of executable work.
 *
 * Each Task stores:
 *   - a pointer to a heap-allocated callable,
 *   - a function pointer to invoke it,
 *   - and a function pointer to destroy it.
 *
 * The design avoids std::function overhead and allows non-throwing, type-erased
 * execution in hot paths. Tasks cannot be move-only because Work-Stealing queues require copyable types.
 */

#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace rts::core {
    /**
     * @brief Represents a type-erased callable used by the runtime.
     *
     * A Task is the basic executable unit within the RTS. It holds a pointer
     * to a heap-allocated callable and the function pointers needed to
     * invoke and destroy it safely. It is intended to be passed around
     * thread-safe queues and executed asynchronously.
     */
    struct Task {
        /// @brief Function pointer type for invoking the stored callable.
        using InvokeFn  = void(*)(void*) noexcept;

        /// @brief Function pointer type for destroying the stored callable.
        using DestroyFn = void(*)(void*) noexcept;

        /// @brief Pointer to the heap-allocated callable.
        void* callable_ptr = nullptr;

        /// @brief Function pointer to invoke the callable.
        InvokeFn invoke_fn = nullptr;

        /// @brief Function pointer to destroy the callable.
        DestroyFn destroy_fn = nullptr;

        /// @brief Default-constructed Task represents an empty/no-op task.
        Task() noexcept = default;

        // Rule of five.

        Task(const Task&) = default;

        Task& operator=(const Task&) = default;

        Task(Task&& other) noexcept = default;

        Task& operator=(Task&& other) noexcept = default;

        /**
         * @brief Constructs a Task from any callable (lambda, functor, etc.).
         * @tparam F Callable type.
         */
        template <typename F>
        requires (  !std::same_as<std::decay_t<F>, Task>) &&
                    std::invocable<std::decay_t<F>&>
            Task(F&& f) noexcept {
            using Fn = std::decay_t<F>;
            callable_ptr = new Fn(std::forward<F>(f));

            assert(callable_ptr && "Task allocation failed");
            invoke_fn = [](void* p) noexcept {
                assert(p && "Invalid function pointer in invoke()");
                (*static_cast<Fn*>(p))();
            };

            destroy_fn = [](void* p) noexcept {
                assert(p && "Invalid function pointer in destroy()");
                delete static_cast<Fn*>(p);
            };
            assert(invoke_fn && destroy_fn && "Invalid function pointers set in Task");
        }

        /**
         * @brief Invokes the stored callable.
         * @note The Task must be valid (non-empty).
         */
        void operator()() const noexcept {
            assert(invoke_fn && "Task::invoke_fn is null");
            assert(callable_ptr && "Task::callable_ptr is null");
            invoke_fn(callable_ptr);
        }

        /**
         * @brief Destroys the stored callable and resets the task to empty.
         */
        void destroy() noexcept {
            assert((callable_ptr && invoke_fn && destroy_fn) &&
           "Invalid Task partial state: missing one of the members");

            destroy_fn(callable_ptr);

            callable_ptr = nullptr;
            invoke_fn = nullptr;
            destroy_fn = nullptr;
        }

        /**
         * @brief Returns true if the Task contains a valid callable.
         */
        explicit operator bool() const noexcept {
            return callable_ptr && invoke_fn && destroy_fn;
        }
    };
} // namespace rts::core