/**
 * @file worker.h
 * @brief Defines the rts::Worker class, representing an individual execution thread
 *        in the MiniRTS runtime system. Each worker maintains its own local queues,
 *        participates in work-stealing, and executes tasks until shutdown.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "constants.h"
#include "task.h"
#include "utils.h"

#include "SPSCQueue.h"
#include "deque.hpp"

namespace core {

    /**
     * @brief Forward declaration for TLS pointer.
     */
    class Worker;

    /**
     * @brief Thread-local pointer to the current executing Worker.
     *        Used by continuations to enqueue locally.
     */
    inline thread_local Worker* tls_worker = nullptr;

    /**
     * @brief Represents a single worker thread in the MiniRTS thread pool.
     *
     * Each Worker owns:
     *  - A work-stealing deque (WSQ) for enqueueing continuations locally
     *      and allowing other Workers to steal from the Worker.
     *  - An SPSC queue (SPSCQ) for tasks submitted externally.
     *  - A dedicated thread executing `run()`, which continually processes tasks.
     *
     * Workers coordinate via shared atomic flags and a global vector of all workers.
     * Each worker can steal tasks from others to balance load.
     *
     * Thread-safe, non-copyable, and movable (to allow storage in std::vector).
     */
    class Worker {
        using WSQ   = riften::Deque<Task>;
        using SPSCQ = rigtorp::SPSCQueue<Task>;

        std::thread thread_;                            ///< The thread executing this worker's main loop.
        std::unique_ptr<WSQ> wsq_;                      ///< Local work-stealing queue.
        std::unique_ptr<SPSCQ> spscq_;                  ///< Single-producer, single-consumer submission queue.
        std::shared_ptr<std::atomic<int>> shutdown_requested_; ///< Shared shutdown flag.
        std::weak_ptr<std::vector<Worker>> workers_vector_;  ///< Shared vector of all workers (for stealing).
        std::shared_ptr<std::atomic<int>> active_workers_;     ///< Tracks number of active workers.
        int core_affinity_;                             ///< Logical CPU core index for pinning.

    public:
        /**
         * @brief Constructs a Worker instance with initialized queues and shared state.
         *
         * @param core_affinity   CPU core index to which the worker will be pinned.
         * @param stop_flag       Shared atomic flag used to signal shutdown.
         * @param queue_capacity  Capacity for both WSQ and SPSC queues.
         * @param workers_vector  Shared vector of all workers.
         * @param active_workers  Shared atomic tracking active worker count.
         */
        Worker(int core_affinity,
               const std::shared_ptr<std::atomic<int>>& stop_flag,
               size_t queue_capacity,
               std::shared_ptr<std::vector<Worker>> workers_vector,
               std::shared_ptr<std::atomic<int>> active_workers) noexcept
            : wsq_(std::make_unique<WSQ>(queue_capacity)),
              spscq_(std::make_unique<SPSCQ>(queue_capacity)),
              shutdown_requested_(stop_flag),
              workers_vector_(std::move(workers_vector)),
              active_workers_(std::move(active_workers)),
              core_affinity_(core_affinity) {
            assert(wsq_ && "Failed to allocate WSQ");
            assert(spscq_ && "Failed to allocate SPSCQ");
            assert(shutdown_requested_ && "Shutdown flag must not be null");
            assert(!workers_vector_.expired() && "workers_vector_ must not be null");
            assert(active_workers_ && "active_workers_ must not be null");
        }

        /**
         * @brief Workers are non-copyable but movable.
         *        Each Worker uniquely owns its thread and local queues.
         *        Movability is required for std::vector<Worker>.
         */
        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
        Worker(Worker&&) noexcept = default;
        Worker& operator=(Worker&&) noexcept = default;

        // ─────────────────────────────────────────────────────────────
        // Query operations
        // ─────────────────────────────────────────────────────────────

        /**
         * @brief Returns the current number of tasks in the local WSQ.
         */
        [[nodiscard]] size_t wsq_size() const noexcept {
            assert(wsq_ && "Work-stealing queue not initialized");
            return wsq_->size();
        }

        /**
         * @brief Attempts to steal a task from this worker’s WSQ.
         * @return An optional Task if stealing succeeded, otherwise std::nullopt.
         */
        [[nodiscard]] std::optional<Task> steal() const noexcept {
            assert(wsq_ && "Work-stealing queue not initialized");
            return wsq_->steal();
        }

        // ─────────────────────────────────────────────────────────────
        // Execution control
        // ─────────────────────────────────────────────────────────────

        /**
         * @brief Starts the worker’s execution loop on its thread.
         *
         * @param num_threads Number of total worker threads in the pool.
         * @note Should only be called once per worker. Call `join()` to wait for termination.
         */
        void run(size_t num_threads = 1) noexcept;

        /**
         * @brief Joins the worker thread, blocking until it finishes execution.
         */
        void join() noexcept {
            if (thread_.joinable())
                thread_.join();
        }

        // ─────────────────────────────────────────────────────────────
        // Task submission
        // ─────────────────────────────────────────────────────────────

        /**
         * @brief Enqueues a task into this worker’s SPSC queue.
         *
         * @param task Task to enqueue.
         * @note Called by the submission (producer) thread. Thread-safe.
         */
        void enqueue(Task&& task) const noexcept {
            assert(task && "Attempting to enqueue an empty Task");
            assert(spscq_ && "SPSC queue not initialized");
            spscq_->emplace(std::move(task));
        }

        /**
         * @brief Enqueues a task locally into this worker’s WSQ.
         *
         * @param task Task to enqueue.
         * @return True if the task was successfully enqueued.
         * @note Used internally by worker threads (e.g., for continuations).
         */
        void enqueue_local(Task&& task) const noexcept {
            assert(task && "Attempting to enqueue an empty Task");
            assert(wsq_ && "Work-stealing queue not initialized");
            wsq_->emplace(std::move(task));
        }
    };

} // namespace core
