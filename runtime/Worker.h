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
        std::shared_ptr<std::vector<Worker>> workers_vector_;
        int core_affinity_;

    public:
        Worker(int core_affinity,
               const std::shared_ptr<std::atomic<int> > &stop_flag,
               size_t queue_capacity,
               std::shared_ptr<std::vector<Worker>> workers_vector) noexcept :
                                                wsq_(std::make_unique<WSQ>(queue_capacity)),
                                                spscq_(std::make_unique<SPSCQ>(queue_capacity)),
                                                shutdown_requested_(stop_flag),
                                                core_affinity_(core_affinity),
                                                workers_vector_(workers_vector){}

        [[nodiscard]] size_t wsq_size() const {
            return wsq_->size();
        }

        [[nodiscard]] std::optional<Task> steal() const {
            return wsq_->steal();
        }

        // TODO: function too long
        void run() noexcept { // TODO: remove work-stealing on single worker
            thread_ = std::thread([this] {
                // size_t largest {0};

                debug_print("Set tls_worker");
                tls_worker = this;
                pin_to_core(core_affinity_);

                // Pointers to facilitate stealing from other workers
                Worker* workers_begin {workers_vector_->data()};
                Worker* workers_end {workers_begin + workers_vector_->size()};
                Worker* next_victim {workers_begin};

                // std::atomic<long> local_counter {0};

                while (shutdown_requested_->load(std::memory_order_relaxed) != HARD_SHUTDOWN) {
                    if (wsq_->empty()) {
                        // Transfer as many items from spscq_ as possible.
                        while (!spscq_->empty() && wsq_->size() != wsq_->capacity()) {
                            wsq_->emplace(*spscq_->front());
                            spscq_->pop();
                        }
                        // largest = std::max(largest, wsq_->size());
                    }
                    std::optional<Task> t = wsq_->pop();
                    if (t.has_value()) {
                        assert(t.value().func);
                        std::osyncstream(std::cout) << core_affinity_ << std::endl;
                        t.value().func();
                        // ++local_counter;
                    } else {
                        // If wsq_ still empty try stealing from another queue.
                        do {
                            ++next_victim;
                            if (next_victim == workers_end)
                                next_victim = workers_begin;
                        } while (next_victim == this);

                        auto victim_queue_size = next_victim->wsq_size();
                        // Steal half their queue.
                        for (int i = 0; i < victim_queue_size/2; i++) {
                            auto stolen_task = next_victim->steal();
                            if (stolen_task.has_value()) {
                                // TODO: remove temp
                                bool result = enqueue_local(stolen_task.value());
                                assert(result);
                                std::osyncstream(std::cout) << "Worker " <<
                                    core_affinity_ << " Stole from " <<
                                        "Worker " << next_victim->core_affinity_ << std::endl;
                            } else {
                                break;
                            }
                        }
                        if (shutdown_requested_->load(std::memory_order_relaxed) == SOFT_SHUTDOWN
                        && wsq_->empty() && spscq_->empty())
                            break;
                    }
                }
                std::osyncstream(std::cout) << "[Exit]: Thread " << core_affinity_ << std::endl
                    // << ", Local counter: " << local_counter <<  std::endl
                    << "[Exit]: Items left in WSQ: " << wsq_->size() << std::endl
                    << "[Exit]: Items left in MPMCQ: " << spscq_->size() << std::endl;
                    // << "[Exit]: Largest: " << largest << std::endl;
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
        [[nodiscard]] bool enqueue_local(const Task& task) const noexcept {
            assert(task.func);
            return wsq_->try_emplace(task);
        }
    };
}
