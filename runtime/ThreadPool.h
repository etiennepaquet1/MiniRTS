#pragma once

#include <random>
#include <thread>
#include <vector>

#include "Worker.h"


namespace rts {

#ifdef __cpp_lib_hardware_interference_size
    inline constexpr std::size_t kCacheLine = std::hardware_destructive_interference_size;
#else
    inline constexpr std::size_t kCacheLine = 64;
#endif


    template <typename T>
    concept ThreadPool = requires(T t, size_t num_threads) {
        {T(num_threads)} noexcept;
        { t.init() } noexcept;
        { t.finalize() } noexcept;
    };


    template<size_t QueueSize = 1024>
    class DefaultThreadPool {
        using QWorker = Worker<QueueSize>;
        std::vector<QWorker> workers_;
        size_t num_threads_;
        std::shared_ptr<std::atomic<bool>> stop_flag_;
        int round_robin_;

    public:
        DefaultThreadPool(size_t num_threads = kCacheLine) noexcept :
            num_threads_(num_threads),
            stop_flag_(std::make_shared<std::atomic<bool>>(false)),
            round_robin_(0) {}
        ~DefaultThreadPool() = default;

        void init() noexcept {
            workers_.reserve(num_threads_);
            for (int i = 0; i < num_threads_; i++) {
                workers_.emplace_back(i, stop_flag_);
                workers_.back().run();
            }
        }

        void finalize() noexcept {
            stop_flag_->store(true, std::memory_order_relaxed);
            for (auto& worker : workers_) {
                worker.join();
            }
        }

        void enqueue(const Task& task) {

        }
    };
}