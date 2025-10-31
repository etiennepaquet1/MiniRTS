#include "worker.h"

void core::Worker::run(size_t num_threads) noexcept {
    active_workers_->fetch_add(1, std::memory_order_release);

    thread_ = std::thread([this, num_threads] {
        pin_to_core(core_affinity_);

        bool active = true;

        // Disable work-stealing for single-worker pools
        bool enable_work_stealing = (num_threads >= 2);

        // Thread-local pointer to self (Used for enqueuing continuations locally).
        tls_worker = this;

        // Pointers to facilitate stealing from other workers
        Worker* workers_begin {workers_vector_->data()};
        Worker* workers_end {workers_begin + num_threads};
        Worker* next_victim {workers_begin};

        while (shutdown_requested_->load(std::memory_order_relaxed) != HARD_SHUTDOWN) {
            if (wsq_->empty()) {
                // Transfer as many items from spscq_ as possible.
                while (!spscq_->empty() && wsq_->size() != wsq_->capacity()) {
                    wsq_->emplace(std::move(*spscq_->front()));
                    spscq_->pop();
                }
            }
            std::optional<Task> t = wsq_->pop();
            if (t.has_value()) {
                assert(t.value());
                t.value()();
                t.value().destroy();
            } else if (enable_work_stealing) {
                // If wsq_ still empty try stealing from another queue.
                do {
                    ++next_victim;
                    if (next_victim == workers_end)
                        next_victim = workers_begin;
                } while (next_victim == this);

                // Approximation of the victim's queue size
                auto victim_queue_size = next_victim->wsq_size();

                // Steal half their queue.
                for (int i = 0; i < victim_queue_size/2; i++) {
                auto stolen_task = next_victim->steal();
                    if (stolen_task.has_value()) {
                        // TODO: remove temp
                        bool result = enqueue_local(std::move(stolen_task.value()));
                        assert(result);
                    } else {
                        break;
                    }
                }
            }
            if (shutdown_requested_->load(std::memory_order_relaxed) == SOFT_SHUTDOWN
                && wsq_->empty() && spscq_->empty()) {
                // Queues are empty and SOFT_SHUTDOWN signal received: Mark worker as inactive.
                if (active) {
                    active = false;
                    active_workers_->fetch_sub(1, std::memory_order_release);
                }
                // Allow worker to steal from other threads until all threads are inactive.
                if (active_workers_->load(std::memory_order_acquire) == 0)
                    break;
            }
        }
        if constexpr (DEBUG) {
            std::osyncstream(std::cout) << "[Exit]: Thread " << core_affinity_ << std::endl
               << "[Exit]: Items left in WSQ: " << wsq_->size() << std::endl
               << "[Exit]: Items left in MPMCQ: " << spscq_->size() << std::endl;
        }
    });
}

