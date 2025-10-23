#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "Task.h"
#include "ThreadPool.h"



namespace rts {

    class Worker;
    class DefaultThreadPool;

    template <typename T>
    class Promise;

    template <typename T>
    class Future;

    inline static std::atomic<bool> running{false};
    inline static void* active_thread_pool = nullptr;

    // Function pointer to enqueue implementation
    inline static void (*enqueue_fn)(const Task&) = nullptr;
    inline static void (*finalize_fn)() = nullptr;

    inline thread_local Worker* tls_worker = nullptr;

    template <ThreadPool T> // TODO: try to find a way to make default thread pool parameter
    bool initialize_runtime(unsigned num_threads = std::thread::hardware_concurrency()) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            auto* pool = new T(num_threads);
            pool->init();

            active_thread_pool = pool;
            enqueue_fn = [](const Task& task) noexcept {
                assert(task.func);
                static_cast<T*>(active_thread_pool)->enqueue(task);
            };
            finalize_fn = []() noexcept {
                auto* p = static_cast<T*>(active_thread_pool);
                if (!p) return;
                p->finalize();      // must join/stop threads here
                delete p;           // not ~T(); use delete
                active_thread_pool = nullptr;
                running.store(false, std::memory_order_release);
            };
            return true;
        }
        return false;
    }

    inline void enqueue(const Task& task) noexcept {
        assert(task.func);
        enqueue_fn(task);
    }

    inline void finalize() noexcept {
        finalize_fn();
    }

    // TODO: make sure the child tasks are enqueued on same worker
    template<typename F, typename... Args>
    auto async(F&& f, Args&&... args) -> Future<std::invoke_result_t<F, Args...>> {
        using T = std::invoke_result_t<F, Args...>;

        Promise<T> p;
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
