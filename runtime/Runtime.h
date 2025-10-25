#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "Task.h"
#include "ThreadPool.h"

namespace rts::async {
    template <typename T>
    class Promise;
    template <typename T>
    class Future;
}

namespace rts {

    class Worker;
    class DefaultThreadPool;

    inline static std::atomic<bool> running{false};
    inline static void* active_thread_pool = nullptr;

    // Function pointer to enqueue() implementation.
    inline static void (*enqueue_fn)(Task&&) = nullptr;
    inline static void (*finalize_fn)() = nullptr;

    template <ThreadPool T> // TODO: try to find a way to make default thread pool parameter
    bool initialize_runtime(unsigned num_threads = std::thread::hardware_concurrency(),
                            unsigned queue_capacity = kDefaultCapacity) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            auto* pool = new T(num_threads, queue_capacity);
            pool->init();

            active_thread_pool = pool;
            enqueue_fn = [](Task&& task) noexcept {
                assert(task.func);
                static_cast<T*>(active_thread_pool)->enqueue(std::move(task));
            };
            finalize_fn = []() noexcept {
                auto* p = static_cast<T*>(active_thread_pool);
                if (!p) return;
                p->finalize();
                delete p;
                active_thread_pool = nullptr;
                running.store(false, std::memory_order_release);
            };
            return true;
        }
        return false;
    }

    inline void enqueue(Task&& task) noexcept {
        assert(task.func);
        enqueue_fn(std::move(task));
    }

    inline void finalize() noexcept {
        finalize_fn();
    }

    // TODO: make sure the child tasks are enqueued on same worker
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
        enqueue(std::move(task));
        return fut;
    }

} // namespace rts
