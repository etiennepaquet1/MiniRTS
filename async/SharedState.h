#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <vector>
#include <functional>

namespace rts::async {

    template <typename T>
    struct SharedState {
        std::mutex mtx;                      // only for blocking wait (optional)
        std::condition_variable cv;          // used only by get()
        std::atomic<bool> ready{false};      // fast path for continuations

        std::optional<T> value;
        std::exception_ptr exception;
        std::vector<std::function<void()>> continuations;  // registered callbacks
    };

    // Same exact concept but remove the "value" member.
    template <>
    struct SharedState<void> {
        std::mutex mtx;                      // only for blocking wait (optional)
        std::condition_variable cv;          // used only by get()
        std::atomic<bool> ready{false};      // fast path for continuations

        std::exception_ptr exception;
        std::vector<std::function<void()>> continuations;  // registered callbacks
    };

}