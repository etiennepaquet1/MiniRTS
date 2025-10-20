#pragma once

#include <future>
#include <random>
#include "ThreadPool.h"

namespace rts {

static std::atomic<bool> running {false};


template<ThreadPool T = DefaultThreadPool<>>
class Runtime {

    static std::unique_ptr<Runtime<>> active_runtime;

    static Runtime& instance(size_t numThreads = std::thread::hardware_concurrency()) {
        if (active_runtime == nullptr) {
            active_runtime = std::make_unique<Runtime<T>>(numThreads);
        }
        std::cout << "warning: returning instance of uninitialized runtime" << std::endl;
        return *active_runtime;
    }

    std::unique_ptr<T> active_pool_ = nullptr;
    unsigned round_robin_ = 0;

    bool initialize(int num_threads = std::thread::hardware_concurrency()) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(
            expected, true,
            std::memory_order_release,
            std::memory_order_relaxed)) {
            active_runtime = std::make_unique<T>(num_threads);
            active_pool_ = std::make_unique<T>(num_threads);
            active_pool_->init();
            return true;
        }
        return false;
    }
public:
    void enqueue(const Task& task) {
        this->active_pool_->enqueue(task);

    }
};

}
