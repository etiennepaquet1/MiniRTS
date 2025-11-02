/**
* @file shared_state.h
 * @brief Defines the shared state used by Promise and Future for communication.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

#include "task.h"

namespace core::async {

    /**
     * @brief Shared state between a Promise<T> and its corresponding Future<T>.
     *
     * This structure holds the result value, readiness flag, stored exception,
     * and continuation tasks registered via Future::then().
     *
     * @tparam T The result type produced by the asynchronous operation.
     */
    template <typename T>
    struct SharedState {
        mutable std::mutex mtx;               ///< Protects access to value and continuations.
        std::atomic<bool> ready{false};       ///< True once value or exception is set.

        std::optional<T> value;               ///< The result value (if successful).
        std::exception_ptr exception;         ///< Exception captured during task execution.
        std::vector<Task> continuations;      ///< Tasks to run once the state becomes ready.
    };

    /**
     * @brief Partial specialization for void-returning tasks.
     *
     * This variant omits the value storage since there’s no return value to store.
     */
    template <>
    struct SharedState<void> {
        mutable std::mutex mtx;
        std::atomic<bool> ready{false};

        std::exception_ptr exception;
        std::vector<Task> continuations;
    };

} // namespace core::async
