#pragma once

#include <thread>

#include "Task.h"
#include "wsq.h"
#include "Utils.h"


template <size_t QueueSize>
class Worker {
    using WSQ = WorkStealingQueue<Task, QueueSize>;

    std::thread thread_;
    std::unique_ptr<WSQ> wsq_;
    std::shared_ptr<std::atomic<bool>> shutdown_requested_;
    int core_affinity_;

public:
    Worker(int core_affinity, std::shared_ptr<std::atomic<bool>> stop_flag)
    : wsq_(std::make_unique<WSQ>()), shutdown_requested_(stop_flag), core_affinity_(core_affinity) {}

    void run() {
        thread_ = std::thread([this] {
            pin_to_core(core_affinity_);
            std::cout << "Thread " << std::this_thread::get_id()
                << " pinned to core " << core_affinity_ << std::endl;
            while (!shutdown_requested_->load(std::memory_order_relaxed)) {
                std::cout << "Worker " << core_affinity_
                          << " on thread " << std::this_thread::get_id() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
    }

    void join() {
        thread_.join();
    }
};

