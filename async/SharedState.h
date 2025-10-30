#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

#include "Task.h"

namespace rts::async {

    template <typename T>
    struct SharedState {
        std::mutex mtx;
        std::atomic<bool> ready{false};

        std::optional<T> value;
        std::exception_ptr exception;
        std::vector<Task> continuations;
    };

    // Same exact concept but remove the "value" member.
    template <>
    struct SharedState<void> {
        std::mutex mtx;
        std::atomic<bool> ready{false};

        std::exception_ptr exception;
        std::vector<Task> continuations;
    };

}