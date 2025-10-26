#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <functional>

#include "Constants.h"
#include "Worker.h"
#include "ThreadPool.h"

namespace rts {

    class DefaultThreadPool {

        std::vector<Worker> workers_;
        size_t num_threads_;
        std::shared_ptr<std::atomic<int>> stop_flag_;
        int round_robin_;
        size_t queue_capacity_;

    public:
        explicit DefaultThreadPool(
            size_t num_threads = std::thread::hardware_concurrency(),
            size_t queue_capacity = kDefaultCapacity) noexcept :
                num_threads_(num_threads),
                stop_flag_(std::make_shared<std::atomic<int> >(0)),
                round_robin_{0},
                queue_capacity_(queue_capacity) {}

        ~DefaultThreadPool() noexcept {
            stop_flag_->store(HARD_SHUTDOWN, std::memory_order_release);
            for (auto& worker : workers_) {
                worker.join();
            }
        }

        void init() noexcept {
            workers_.reserve(num_threads_);
            for (int i = 0; i < num_threads_; i++) {
                workers_.emplace_back(i, stop_flag_, queue_capacity_);
                workers_.back().run();
            }
        }

        void finalize(ShutdownMode mode) noexcept {
            stop_flag_->store(mode, std::memory_order_release);
            for (auto& worker : workers_) {
                worker.join();
            }
        }

        void enqueue(const Task& task) noexcept {
            assert(task.func);
            workers_[round_robin_].enqueue(task);
            round_robin_++;
            if (round_robin_ == num_threads_)
                round_robin_ = 0;
        }
    };

    static_assert(ThreadPool<DefaultThreadPool>);
}