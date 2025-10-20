#pragma once

#include <future>
#include "ThreadPool.h"

namespace rts {

    inline static std::atomic<bool> running{false};
    inline static void* active_runtime = nullptr;

    // Function pointer to enqueue implementation
    inline static void (*enqueue_fn)(void*, const Task&) = nullptr;

    template <ThreadPool T = DefaultThreadPool>
    bool initialize_runtime(int num_threads = std::thread::hardware_concurrency()) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            auto* pool = new T(num_threads);
            pool->init();

            active_runtime = pool;
            enqueue_fn = [](void* pool_ptr, const Task& task) noexcept {
                static_cast<T*>(pool_ptr)->enqueue(task);
            };
            return true;
        }
        return false;
    }

    inline void enqueue(const Task& task) noexcept {
        enqueue_fn(active_runtime, task);
    }

    inline void finalize() noexcept {
        // Optional cleanup
        running.store(false, std::memory_order_release);
    }

} // namespace rts



// inline static std::unique_ptr<AnyThreadPool> active_thread_pool {nullptr};
//
// template <ThreadPool T = DefaultThreadPool>
// static bool initialize_runtime(int num_threads = std::thread::hardware_concurrency()) noexcept {
//     bool expected = false;
//     if (running.compare_exchange_strong(expected, true,
//                                     std::memory_order_release,
//                                     std::memory_order_relaxed)) {
//             active_thread_pool = std::make_unique<AnyThreadPool>(std::make_unique<T>(num_threads));
//             active_thread_pool->init();
//             return true;
//         }
//     return false;
// }
//
// template <ThreadPool T = DefaultThreadPool>
// static T& instance() {
//     // Return pointer to active thread pool
//     if (!running.load(std::memory_order_acquire)) {
//         initialize_runtime();
//     }
//     return *active_thread_pool.get();
// }
//
// inline void enqueue(const Task& task) {
//     active_thread_pool->enqueue(task);
// }

