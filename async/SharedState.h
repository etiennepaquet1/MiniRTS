#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>
#include <functional>

#include "Task.h"

namespace rts::async {

    template <typename T>
    struct SharedState {
        std::mutex mtx;                      // only for blocking wait (optional)
        std::atomic<bool> ready{false};      // fast path for continuations

        std::optional<T> value;
        std::exception_ptr exception;
        std::vector<Task> continuations;  // registered callbacks
    };

    // Same exact concept but remove the "value" member.
    template <>
    struct SharedState<void> {
        std::mutex mtx;                      // only for blocking wait (optional)
        std::atomic<bool> ready{false};      // fast path for continuations

        std::exception_ptr exception;
        std::vector<Task> continuations;  // registered callbacks
    };

}