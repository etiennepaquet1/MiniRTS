#pragma once

#include <syncstream>
#include <thread>

#include "Task.h"
#include "WorkStealingQueue/include/WorkStealingQueue.h"
#include "SPSCQueue/include/rigtorp/SPSCQueue.h"
#include "Utils.h"


namespace rts {

    class Worker;
    inline thread_local Worker* tls_worker = nullptr;

    class Worker {
        using WSQ = WorkStealingQueue<Task>;
        using SPSCQ = rigtorp::SPSCQueue<Task>;

        std::thread thread_;
        std::unique_ptr<WSQ> wsq_;
        std::unique_ptr<SPSCQ> spscq_;
        std::shared_ptr<std::atomic<int>> shutdown_requested_;
        int core_affinity_;

    public:
        Worker(int core_affinity,
               const std::shared_ptr<std::atomic<int> > &stop_flag,
               size_t queue_capacity) noexcept : wsq_(std::make_unique<WSQ>(queue_capacity)),
                                                 spscq_(std::make_unique<SPSCQ>(queue_capacity)),
                                                 shutdown_requested_(stop_flag),
                                                 core_affinity_(core_affinity) {}

        void run() noexcept {
            thread_ = std::thread([this] {
                size_t largest {0};

                debug_print("Set tls_worker");
                tls_worker = this;
                pin_to_core(core_affinity_);

                std::atomic<long> local_counter {0};

                while (shutdown_requested_->load(std::memory_order_relaxed) != HARD_SHUTDOWN) {
                    if (wsq_->empty()) {
                        // Transfer as many items from spscq_ as possible.
                        while (!spscq_->empty() && wsq_->size() != wsq_->capacity()) {
                            wsq_->emplace(*spscq_->front());
                            spscq_->pop();
                        }
                        largest = std::max(largest, wsq_->size());
                        // if wsq_ still empty try stealing from other queues
                    }
                    std::optional<Task> t = wsq_->pop();
                    if (t.has_value()) {
                        assert(t.value().func);
                        t.value().func();
                        ++local_counter;
                    } else if (shutdown_requested_->load(std::memory_order_relaxed) == SOFT_SHUTDOWN
                               && wsq_->empty() && spscq_->empty())
                        break;
                }
                std::osyncstream(std::cout) << "[Exit]: Thread " << core_affinity_
                    << ", Local counter: " << local_counter <<  std::endl
                    << "[Exit]: Items left in WSQ: " << wsq_->size() << std::endl
                    << "[Exit]: Items left in MPMCQ: " << spscq_->size() << std::endl
                    << "[Exit]: Largest: " << largest << std::endl;
            });
        }

        void join() noexcept {
            if (thread_.joinable())
                thread_.join();
        }

        // Used by the submission thread to enqueue into the worker's SPSC queue.
        void enqueue(const Task& task) const noexcept {
            assert(task.func);
            spscq_->push(task);
        }

        // Used by the worker thread to enqueue continuations to its own work-stealing queue.
        bool enqueue_local(const Task& task) const noexcept {
            assert(task.func);
            return wsq_->try_emplace(task);
        }
    };
}
