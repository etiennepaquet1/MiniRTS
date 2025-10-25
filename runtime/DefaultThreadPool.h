#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <functional>

#include "Constants.h"
#include "Worker.h"
#include "ThreadPool.h"

namespace rts {
    inline std::atomic<int> threadpool_enqueue {0};

    class DefaultThreadPool {

        std::vector<Worker> workers_;
        size_t num_threads_;
        std::shared_ptr<std::atomic<bool>> stop_flag_;
        int round_robin_;
        size_t queue_capacity_;

    public:
        explicit DefaultThreadPool(
            size_t num_threads = std::thread::hardware_concurrency(),
            size_t queue_capacity = kDefaultCapacity) noexcept :
                num_threads_(num_threads),
                stop_flag_(std::make_shared<std::atomic<bool> >(false)),
                round_robin_(0),
                queue_capacity_(queue_capacity) {}

        ~DefaultThreadPool() noexcept {
            stop_flag_->store(true);
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

        void finalize() noexcept {
            stop_flag_->store(true, std::memory_order_relaxed);
            for (auto& worker : workers_) {
                worker.join();
            }
        }

        void enqueue(Task&& task) noexcept {
            assert(task.func);
            ++threadpool_enqueue;
            // round robin until one of the queues has space
            while (!workers_[round_robin_].try_enqueue(std::move(task))) {
                round_robin_++;
                if (round_robin_ == num_threads_)
                    round_robin_ = 0;
            }
            // Avoid picking same Worker multiple times in a row.
            round_robin_++;
            if (round_robin_ == num_threads_)
                round_robin_ = 0;
        }
    };

    static_assert(ThreadPool<DefaultThreadPool>);
}