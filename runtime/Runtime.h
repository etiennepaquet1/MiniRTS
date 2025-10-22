#pragma once

#include <future>
#include "ThreadPool.h"

namespace rts {

    inline static std::atomic<bool> running{false};
    inline static void* active_thread_pool = nullptr;

    // Function pointer to enqueue implementation
    inline static void (*enqueue_fn)(const Task&) = nullptr;
    inline static void (*finalize_fn)() = nullptr;

    template <ThreadPool T = DefaultThreadPool>
    bool initialize_runtime(int num_threads = std::thread::hardware_concurrency()) noexcept {
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

} // namespace rts
