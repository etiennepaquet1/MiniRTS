#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <syncstream>
#include <thread>
#include <utility>
#include <vector>

#include "Constants.h"
#include "SPSCQueue/include/rigtorp/SPSCQueue.h"
#include "Task.h"
#include "Utils.h"
#include "WorkStealingQueue/include/WorkStealingQueue.h"


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
        std::shared_ptr<std::atomic<int>> active_workers_;
        int core_affinity_;

    public:
        Worker(int core_affinity,
               const std::shared_ptr<std::atomic<int> > &stop_flag,
               size_t queue_capacity,
               std::shared_ptr<std::vector<Worker>> workers_vector,
               std::shared_ptr<std::atomic<int>> active_workers) noexcept :
                                                wsq_(std::make_unique<WSQ>(queue_capacity)),
                                                spscq_(std::make_unique<SPSCQ>(queue_capacity)),
                                                shutdown_requested_(stop_flag),
                                                core_affinity_(core_affinity),
                                                workers_vector_(workers_vector),
                                                active_workers_(active_workers){}

        [[nodiscard]] size_t wsq_size() const {
            return wsq_->size();
        }

        [[nodiscard]] std::optional<Task> steal() const {
            return wsq_->steal();
        }

        // TODO: function way too long
        // TODO: use [[likely]]
        void run(size_t num_threads = 1) noexcept;
        // {
        //     active_workers_->fetch_add(1, std::memory_order_release);
        //
        //     thread_ = std::thread([this, num_threads] {
        //         pin_to_core(core_affinity_);
        //
        //         bool active = true;
        //
        //         // Disable work-stealing for single-worker pools
        //         bool enable_work_stealing = (num_threads >= 2);
        //
        //         // Thread-local pointer to self (Used for enqueuing continuations locally).
        //         tls_worker = this;
        //
        //         // Pointers to facilitate stealing from other workers
        //         Worker* workers_begin {workers_vector_->data()};
        //         Worker* workers_end {workers_begin + num_threads};
        //         Worker* next_victim {workers_begin};
        //
        //         while (shutdown_requested_->load(std::memory_order_relaxed) != HARD_SHUTDOWN) {
        //             if (wsq_->empty()) {
        //                 // Transfer as many items from spscq_ as possible.
        //                 while (!spscq_->empty() && wsq_->size() != wsq_->capacity()) {
        //                     wsq_->emplace(std::move(*spscq_->front()));
        //                     spscq_->pop();
        //                 }
        //             }
        //             std::optional<Task> t = wsq_->pop();
        //             if (t.has_value()) {
        //                 assert(t.value());
        //                 t.value()();
        //                 t.value().destroy();
        //             } else if (enable_work_stealing) {
        //                 // If wsq_ still empty try stealing from another queue.
        //                 do {
        //                     ++next_victim;
        //                     if (next_victim == workers_end)
        //                         next_victim = workers_begin;
        //                 } while (next_victim == this);
        //
        //                 // Approximation of the victim's queue size
        //                 auto victim_queue_size = next_victim->wsq_size();
        //
        //                 // Steal half their queue.
        //                 for (int i = 0; i < victim_queue_size/2; i++) {
        //                 auto stolen_task = next_victim->steal();
        //                     if (stolen_task.has_value()) {
        //                         // TODO: remove temp
        //                         bool result = enqueue_local(std::move(stolen_task.value()));
        //                         assert(result);
        //                     } else {
        //                         break;
        //                     }
        //                 }
        //             }
        //             if (shutdown_requested_->load(std::memory_order_relaxed) == SOFT_SHUTDOWN
        //                 && wsq_->empty() && spscq_->empty()) {
        //                 // Queues are empty and SOFT_SHUTDOWN signal received: Mark worker as inactive.
        //                 if (active) {
        //                     active = false;
        //                     active_workers_->fetch_sub(1, std::memory_order_release);
        //                 }
        //                 // Allow worker to steal from other threads until all threads are inactive.
        //                 if (active_workers_->load(std::memory_order_acquire) == 0)
        //                     break;
        //             }
        //         }
        //         std::osyncstream(std::cout) << "[Exit]: Thread " << core_affinity_ << std::endl
        //             << "[Exit]: Items left in WSQ: " << wsq_->size() << std::endl
        //             << "[Exit]: Items left in MPMCQ: " << spscq_->size() << std::endl;
        //     });
        // }

        void join() noexcept {
            if (thread_.joinable())
                thread_.join();
        }

        // Used by the submission thread to enqueue into the worker's SPSC queue.
        void enqueue(Task&& task) const noexcept {
            assert(task);
            spscq_->emplace(std::move(task));
        }

        // Used by the worker thread to enqueue continuations to its own work-stealing queue.
        [[nodiscard]] bool enqueue_local(Task &&task) const noexcept {
            assert(task);
            return wsq_->try_emplace(std::move(task));
        }
    };
}
