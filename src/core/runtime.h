/**
 * @file runtime.h
 * @brief Defines global runtime control functions for MiniRTS, including initialization,
 *        task submission, and shutdown. Provides generic runtime management that can work
 *        with any thread pool type satisfying the ThreadPool concept.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "concepts.h"
#include "task.h"
#include "thread_pool.h"
#include "constants.h"

namespace core::async {
    template <typename T>
    class Promise;

    template <typename T>
    requires concepts::FutureValue<T>
    class Future;
}

namespace core {
    class DefaultThreadPool;

    // ─────────────────────────────────────────────────────────────
    // ───────────────  Global runtime state and hooks  ─────────────
    // ─────────────────────────────────────────────────────────────

    /// @brief Flag indicating whether the RTS is currently active.
    inline std::atomic<bool> running{false};

    /// @brief Opaque pointer to the currently active thread pool instance.
    inline void* active_thread_pool = nullptr;

    /// @brief Function pointer bound to the runtime’s active enqueue() implementation.
    inline void (*enqueue_fn)(Task&&) = nullptr;

    /// @brief Function pointer bound to the runtime’s active finalize() implementation.
    inline void (*finalize_fn)(ShutdownMode mode) = nullptr;

    /// @brief Cached saturation metric for monitoring queue load (optional diagnostic).
    inline float saturation_cached = 0.0f;
} // namespace core

namespace rts
{
    // ─────────────────────────────────────────────────────────────
    // ────────────────  Runtime initialization API  ────────────────
    // ─────────────────────────────────────────────────────────────

    /**
     * @brief Initializes the MiniRTS runtime with the given thread pool type.
     *
     * @tparam T ThreadPool implementation type (must satisfy ThreadPool concept).
     * @param num_threads Number of worker threads to spawn.
     * @param queue_capacity Per-worker queue capacity.
     * @return True if initialization succeeded, false if a runtime was already running.
     *
     * @note This function allocates the runtime pool on the heap and binds
     *       function pointers for enqueueing and finalization.
     */
    template <core::ThreadPool T = core::DefaultThreadPool>
    bool initialize_runtime(size_t num_threads = std::thread::hardware_concurrency(),
                            size_t queue_capacity = core::kDefaultCapacity) noexcept {
        bool expected = false;

        // Ensure only one runtime instance is active at a time.
        if (core::running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            assert(!core::active_thread_pool && "Runtime already has an active thread pool");
            assert(!core::enqueue_fn && !core::finalize_fn && "Function pointers must be null before init");

            auto* pool = new T(num_threads, queue_capacity);
            pool->init();  // User-defined startup logic for the pool.

            core::active_thread_pool = pool;

            // Bind enqueue function pointer
            core::enqueue_fn = [](Task&& task) noexcept {
                assert(core::active_thread_pool && "No active thread pool set");
                assert(task && "Attempting to enqueue an empty task");
                static_cast<T*>(core::active_thread_pool)->enqueue(std::move(task));
            };

            // Bind finalize function pointer
            core::finalize_fn = [](core::ShutdownMode mode) noexcept {
                auto* p = static_cast<T*>(core::active_thread_pool);
                if (!p) {
                    assert(false && "finalize_fn called with null thread pool");
                    return;
                }
                p->finalize(mode);
                delete p;

                core::active_thread_pool = nullptr;
                core::enqueue_fn = nullptr;
                core::finalize_fn = nullptr;
                core::running.store(false, std::memory_order_release);
            };

            return true;
        }

        return false;  // Already initialized
    }

    // ─────────────────────────────────────────────────────────────
    // ────────────────  Runtime finalization API  ─────────────────
    // ─────────────────────────────────────────────────────────────

    /**
     * @brief Immediately stops all workers and releases runtime resources.
     * @note Tasks currently running may be terminated abruptly.
     */
    inline void finalize_hard() noexcept {
        assert(core::finalize_fn && "finalize_hard() called before initialization");
        core::finalize_fn(core::HARD_SHUTDOWN);
    }

    /**
     * @brief Gracefully shuts down the runtime after all active tasks complete.
     * @note Queued tasks are completed before shutdown.
     */
    inline void finalize_soft() noexcept {
        assert(core::finalize_fn && "finalize_soft() called before initialization");
        core::finalize_fn(core::SOFT_SHUTDOWN);
    }

    // ─────────────────────────────────────────────────────────────
    // ──────────────────────  Task Enqueue API  ───────────────────
    // ─────────────────────────────────────────────────────────────

    /**
     * @brief Enqueues a task into the runtime’s active thread pool.
     *
     * @param task Callable wrapped as rts::Task.
     * @note This function dispatches via the pool-specific enqueue_fn set at initialization.
     */
    inline void enqueue(Task&& task) noexcept {
        assert(core::running.load(std::memory_order_acquire) && "enqueue() called on inactive runtime");
        assert(core::enqueue_fn && "enqueue() called before initialization");
        assert(task && "Attempting to enqueue an empty task");
        core::enqueue_fn(std::move(task));
    }

    /**
     * @brief Asynchronously enqueues a callable for execution and returns a Future for its result.
     *
     * @tparam F Callable type.
     * @tparam Args Argument pack for callable.
     * @return async::Future<T> representing the result.
     *
     * @note The callable is executed inside the runtime’s thread pool.
     */
    template<typename F, typename... Args>
    auto async(F&& f, Args&&... args)
        -> core::async::Future<std::invoke_result_t<F, Args...>> {

        using T = std::invoke_result_t<F, Args...>;

        assert(core::running.load(std::memory_order_acquire) && "enqueue_async() called on inactive runtime");
        assert(core::enqueue_fn && "enqueue_async() called before initialization");

        core::async::Promise<T> p;
        auto fut = p.get_future();

        // Capture the promise by value (moved)
        Task task = [func = std::forward<F>(f),
                     args_tuple = std::make_tuple(std::forward<Args>(args)...),
                     p = std::move(p)]() mutable {
            try {
                if constexpr (std::is_void_v<T>) {
                    std::apply(func, std::move(args_tuple));
                    p.set_value();
                } else {
                    T result = std::apply(func, std::move(args_tuple));
                    p.set_value(std::move(result));
                }
            } catch (...) {
                p.set_exception(std::current_exception());
            }
        };

        enqueue(std::move(task));
        return fut;
    }

    // ─────────────────────────────────────────────────────────────
    // ────────────────────────  when_all()  ───────────────────────
    // ─────────────────────────────────────────────────────────────

    /**
     * @brief Combines multiple futures into one future that resolves
     *        when all input futures have completed successfully.
     *
     * @tparam Futures Variadic pack of async::Future<T> types.
     * @param futures The input futures to wait on.
     * @return async::Future<std::tuple<...>> containing all results.
     *
     * @note This overload does not yet handle Future<void> types.
     *       Use the specialized version (TODO) when available.
     */
    template <typename... Futures>
    auto when_all(Futures&&... futures) {
        using ResultTuple = std::tuple<typename std::decay_t<Futures>::value_type...>;
        using StateTuple  = std::tuple<std::optional<typename std::decay_t<Futures>::value_type>...>;

        core::async::Promise<ResultTuple> prom;
        auto out = prom.get_future();

        constexpr std::size_t N = sizeof...(Futures);
        if constexpr (N == 0) {
            prom.set_value(ResultTuple{});
            return out;
        }

        auto state     = std::make_shared<StateTuple>();
        auto remaining = std::make_shared<std::atomic<std::size_t>>(N);

        // Called after each future completes; builds result tuple when last finishes.
        auto fulfill = [prom = std::move(prom), state, remaining]() mutable {
            assert(remaining && "Remaining counter null");
            assert(state && "State tuple null");
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                auto result = std::apply(
                    []<typename... Ts>(std::optional<Ts>&... opts) -> ResultTuple {
                        // ((assert(opts.has_value() && "Missing value in when_all")), ...);
                        return ResultTuple{ std::move(*opts)... };
                    },
                    *state
                );
                prom.set_value(std::move(result));
            }
        };

        // Attach continuations per input future.
        auto attach_one = [state, fulfill](auto& fut, [[maybe_unused]] auto index_c) {
            constexpr std::size_t I = decltype(index_c)::value;
            fut.then([state, fulfill](auto&& v) mutable {
                std::get<I>(*state).emplace(std::forward<decltype(v)>(v));
                fulfill();
            });
        };

        // Generate indices [0..N) and attach
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (attach_one(futures, std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Futures...>{});

        return out;
    }

} // namespace rts
