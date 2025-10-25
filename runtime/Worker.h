#pragma once

#include <syncstream>
#include <thread>

#include "Task.h"
#include "WorkStealingQueue/include/WorkStealingQueue.h"
#include "MPMCQueue/include/rigtorp/MPMCQueue.h"
#include "Utils.h"


namespace rts {
    inline std::atomic<int> spsc_enqueues {0};
    inline std::atomic<int> wsq_pops {0};

    class Worker;
    inline thread_local Worker* tls_worker = nullptr;

    class Worker {
        using WSQ = WorkStealingQueue<Task>;
        using MPMCQ = rigtorp::mpmc::Queue<Task>;

        std::thread thread_;
        std::unique_ptr<WSQ> wsq_;
        std::unique_ptr<MPMCQ> mpmcq_;
        std::shared_ptr<std::atomic<bool>> shutdown_requested_;
        int core_affinity_;

    public:
        Worker(int core_affinity,
               const std::shared_ptr<std::atomic<bool> > &stop_flag,
               size_t queue_capacity) noexcept : wsq_(std::make_unique<WSQ>(queue_capacity)),
                                                 mpmcq_(std::make_unique<MPMCQ>(queue_capacity)),
                                                 shutdown_requested_(stop_flag),
                                                 core_affinity_(core_affinity) {}

        void run() noexcept {
            thread_ = std::thread([this] {
                debug_print("Set tls_worker");
                tls_worker = this;
                pin_to_core(4); //TODO: revert back to core_affinity

                std::atomic<long> local_counter {0};

                while (!shutdown_requested_->load(std::memory_order_relaxed)) {
                    if (wsq_->empty()) {
                        // Transfer as many items from spscq_ as possible.
                        while (!mpmcq_->empty() && wsq_->size() != wsq_->capacity()) {
                            Task t{};
                            mpmcq_->pop(t);
                            wsq_->emplace(std::move(t));
                        }
                        // if wsq_ still empty try stealing from other queues
                    }
                    std::optional<Task> t = wsq_->pop();
                    if (t.has_value()) {
                        ++wsq_pops;
                        assert(t.value().func);
                        t.value().func();
                        ++local_counter;
                    }
                }
                std::osyncstream(std::cout) << "[Exit]: Thread " << core_affinity_
                    << ", Local counter: " << local_counter <<  std::endl;
                std::osyncstream(std::cout) << "[Exit]: Items left in WSQ: " << wsq_->size() << std::endl;
                std::osyncstream(std::cout) << "[Exit]: Items left in MPMCQ: " << mpmcq_->size() << std::endl;
            });
        }

        void join() noexcept {
            if (thread_.joinable())
                thread_.join();
        }

        void enqueue(Task&& task) const noexcept {
            assert(task.func);
            mpmcq_->push(std::move(task));
        }

        [[nodiscard]] bool try_enqueue(Task&& task) const noexcept {
            assert(task.func);
            bool result = mpmcq_->try_push(std::move(task));
            if (result) ++spsc_enqueues;
            return result;
        }
    };
}
