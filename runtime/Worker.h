#pragma once

#include <syncstream>
#include <thread>

#include "Task.h"
#include "WorkStealingQueue/include/WorkStealingQueue.h"
#include "Utils.h"


class Worker {
    using WSQ = WorkStealingQueue<Task>;
    using SPSCQ = rigtorp::SPSCQueue<Task>;

    std::thread thread_;
    std::unique_ptr<WSQ> wsq_;
    std::unique_ptr<SPSCQ> spscq_;
    std::shared_ptr<std::atomic<bool>> shutdown_requested_;
    int core_affinity_;

public:
    Worker(int core_affinity, std::shared_ptr<std::atomic<bool>> stop_flag, size_t queue_capacity):
        wsq_(std::make_unique<WSQ>(queue_capacity)),
        spscq_(std::make_unique<SPSCQ>(queue_capacity)),
        shutdown_requested_(stop_flag),
        core_affinity_(core_affinity) {}

    void run() {
        thread_ = std::thread([this] {
            pin_to_core(core_affinity_);
            long local_counter {0};
            while (!shutdown_requested_->load(std::memory_order_relaxed)) {
                if (wsq_->empty()) {
                    while (!spscq_->empty() && wsq_->size() != wsq_->capacity()) {
                        wsq_->emplace(*spscq_->front());
                        spscq_->pop();
                    }
                    // if wsq_ still empty try stealing from other queues
                }
                std::optional<Task> t = wsq_->pop();
                if (t.has_value()) {
                    assert(t.value().func);
                    t.value().func();
                    ++local_counter;
                }
            }
        });
    }

    void join() {
        thread_.join();
    }

    void enqueue(const Task& task) noexcept {
        assert(task.func);
        spscq_->push(task);
    }
};


