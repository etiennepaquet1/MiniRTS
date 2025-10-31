/**
 * @file default_thread_pool.h
 * @brief Defines the default thread pool implementation for the RTS runtime system.
 *
 * This thread pool manages a group of Worker threads that execute submitted Tasks.
 * Tasks are distributed in a round-robin fashion across the workers' local queues.
 *
 * @note The pool supports both hard and soft shutdown modes via the shared stop_flag_.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <syncstream>

#include "constants.h"
#include "task.h"
#include "thread_pool.h"
#include "worker.h"
#include "utils.h"

namespace core {

    /**
     * @brief The default thread pool used by MiniRTS.
     *
     * This pool owns and manages a fixed number of Worker threads.
     * Each Worker maintains its own work-stealing queue, and tasks are
     * enqueued in a round-robin pattern to balance load.
     *
     * The pool can be safely initialized and finalized multiple times
     * (using soft or hard shutdown), and supports basic saturation metrics.
     */
    class DefaultThreadPool {

        std::shared_ptr<std::vector<Worker>> workers_;          ///< Managed worker threads.
        size_t num_threads_;                                    ///< Number of threads in the pool.
        std::shared_ptr<std::atomic<int>> stop_flag_;           ///< Shared shutdown flag.
        std::shared_ptr<std::atomic<int>> active_workers_;      ///< Count of currently active workers.
        int round_robin_;                                       ///< Index for round-robin scheduling.
        size_t queue_capacity_;                                 ///< Per-worker queue capacity.


    public:
        /**
         * @brief Constructs a thread pool with the given number of threads and queue capacity.
         * @param num_threads Number of worker threads to spawn (defaults to hardware concurrency).
         * @param queue_capacity Capacity for each worker’s local queue.
         */
        explicit DefaultThreadPool(
            size_t num_threads = std::max(std::thread::hardware_concurrency(), 1u),
            size_t queue_capacity = kDefaultCapacity) noexcept
            : workers_(std::make_shared<std::vector<Worker>>()),
              num_threads_(num_threads),
              stop_flag_(std::make_shared<std::atomic<int>>(0)),
              active_workers_(std::make_shared<std::atomic<int>>(0)),
              round_robin_(0),
              queue_capacity_(queue_capacity)
        {
            assert(num_threads_ > 0 && "ThreadPool must have at least one thread");
            assert(queue_capacity_ > 0 && "Queue capacity must be non-zero");
        }

        /**
         * @brief Disable copy and move semantics — thread pools cannot be safely duplicated or transferred.
         */
        DefaultThreadPool(const DefaultThreadPool&) = delete;             ///< Non-copyable
        DefaultThreadPool& operator=(const DefaultThreadPool&) = delete;  ///< Non-copy-assignable
        DefaultThreadPool(DefaultThreadPool&&) = delete;                  ///< Non-movable
        DefaultThreadPool& operator=(DefaultThreadPool&&) = delete;       ///< Non-move-assignable


        /**
         * @brief Destructor — performs a hard shutdown and joins all workers.
         */
        ~DefaultThreadPool() noexcept {
            if (!workers_ || workers_->empty()) return;

            stop_flag_->store(HARD_SHUTDOWN, std::memory_order_release);
            for (auto& worker : *workers_) {
                worker.join();
            }
        }

        /**
         * @brief Initializes and launches all worker threads.
         *
         * Must be called before enqueuing any tasks.
         */
        void init() noexcept {
            assert(workers_ && "Workers vector not allocated");
            assert(workers_->empty() && "ThreadPool::init() called twice without finalize()");
            assert(num_threads_ > 0);

            workers_->reserve(num_threads_);
            for (size_t i = 0; i < num_threads_; ++i) {
                workers_->emplace_back(
                    static_cast<int>(i),
                    stop_flag_,
                    queue_capacity_,
                    workers_,
                    active_workers_);
            }

            for (size_t i = 0; i < num_threads_; ++i) {
                (*workers_)[i].run(num_threads_);
            }

            assert(workers_->size() == num_threads_ && "Worker initialization incomplete");
        }

        /**
         * @brief Requests a shutdown and waits for all workers to finish.
         * @param mode Shutdown mode: HARD_SHUTDOWN or SOFT_SHUTDOWN.
         */
        void finalize(ShutdownMode mode) const noexcept {
            assert(workers_ && "finalize() called before init()");
            assert(!workers_->empty() && "finalize() called with no active workers");
            stop_flag_->store(mode, std::memory_order_release);

            for (auto& worker : *workers_) {
                worker.join();
            }
        }

        /**
         * @brief Computes a simple saturation metric across all workers' queues.
         *
         * @return Approximate ratio of total enqueued tasks per worker.
         */
        [[nodiscard]] double compute_saturation() const noexcept {
            assert(workers_ && "compute_saturation() called before init()");
            assert(!workers_->empty() && "compute_saturation() called on empty worker set");

            size_t sum {0};
            for (Worker& wkr : *workers_) {
                sum += wkr.wsq_size();
            }

            const double total = static_cast<double>(workers_->size() * queue_capacity_);
            assert(total > 0.0);
            return static_cast<double>(sum) / total;
        }

        /**
         * @brief Enqueues a Task into the next worker’s local queue.
         *
         * Tasks are assigned in round-robin order for load distribution.
         * @param task The task to enqueue.
         */
        void enqueue(Task &&task) noexcept {
            assert(workers_ && "enqueue() called before init()");
            assert(!workers_->empty() && "enqueue() called on empty ThreadPool");
            assert(task && "enqueue() received an empty Task");

            (*workers_)[round_robin_].enqueue(std::move(task));

            round_robin_++;
            if (round_robin_ >= static_cast<int>(num_threads_)) {
                round_robin_ = 0;
            }
        }
    };

    static_assert(ThreadPool<DefaultThreadPool>,
                  "DefaultThreadPool must satisfy the ThreadPool concept");
} // namespace rts
