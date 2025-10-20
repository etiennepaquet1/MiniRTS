#pragma once

#include <syncstream>
#include <thread>

#include "Task.h"
#include "WorkStealingQueue/include/WorkStealingQueue.h"
#include "Utils.h"


class Worker {
    using WSQ = WorkStealingQueue<Task>;

    std::thread thread_;
    std::unique_ptr<WSQ> wsq_;
    std::shared_ptr<std::atomic<bool>> shutdown_requested_;
    int core_affinity_;

public:
    Worker(int core_affinity, std::shared_ptr<std::atomic<bool>> stop_flag, size_t queue_capacity):
        wsq_(std::make_unique<WSQ>(queue_capacity)),
        shutdown_requested_(stop_flag),
        core_affinity_(core_affinity) {}

    void run() {
        thread_ = std::thread([this] {
            pin_to_core(core_affinity_);
            // std::osyncstream(std::cout) << "Thread " << std::this_thread::get_id()
            //     << " pinned to core " << core_affinity_ << std::endl;
            while (!shutdown_requested_->load(std::memory_order_relaxed)) {
                // std::osyncstream(std::cout) << "Worker " << core_affinity_
                //           << " on thread " << std::this_thread::get_id() << std::endl;
                if (!wsq_->empty()) {
                    std::optional<Task> t = wsq_->pop();
                    if (t.has_value()) {
                        t.value().func();
                    }
                }
            }
        });
    }

    void join() {
        thread_.join();
    }

    void enqueue(const Task& task) noexcept {
        wsq_->emplace(task);
    }
};


