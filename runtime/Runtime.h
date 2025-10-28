#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "Task.h"
#include "ThreadPool.h"
#include "Constants.h"

namespace rts::async {
    template <typename T>
    class Promise;
    template <typename T>
    class Future;
}

namespace rts {

    class DefaultThreadPool;

    inline static std::atomic<bool> running{false};
    inline static void* active_thread_pool = nullptr;

    // Function pointer to enqueue() implementation.
    inline static void (*enqueue_fn)(const Task&) = nullptr;
    inline static void (*finalize_fn)(ShutdownMode mode) = nullptr;

    // Represents how saturated the worker queues are
    inline float saturation_cached = 0;
    // Worker queue capacity (Used for calculating saturation)

    template <ThreadPool T> // TODO: try to find a way to make default thread pool parameter
    bool initialize_runtime(size_t num_threads = std::thread::hardware_concurrency(),
                            size_t queue_capacity = kDefaultCapacity) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            auto* pool = new T(num_threads, queue_capacity);
            pool->init();

            active_thread_pool = pool;
            enqueue_fn = [](const Task& task) noexcept {
                assert(task.func);
                static_cast<T*>(active_thread_pool)->enqueue(std::move(task));
            };
            finalize_fn = [](ShutdownMode mode) noexcept {
                auto* p = static_cast<T*>(active_thread_pool);
                if (!p) return;
                p->finalize(mode);
                delete p;
                active_thread_pool = nullptr;
                running.store(false, std::memory_order_release);
            };
            return true;
        }
        return false;
    }

    inline void enqueue(const Task& task) noexcept {
        assert(task.func);
        enqueue_fn(std::move(task));
    }

    inline void finalize_hard() noexcept {
        finalize_fn(HARD_SHUTDOWN);
    }
    inline void finalize_soft() noexcept {
        finalize_fn(SOFT_SHUTDOWN);
    }

    template<typename F, typename... Args>
    auto enqueue_async(F&& f, Args&&... args) -> async::Future<std::invoke_result_t<F, Args...>> {
        using T = std::invoke_result_t<F, Args...>;

        async::Promise<T> p;
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
        enqueue(task);
        return fut;
    }

} // namespace rts
