#pragma once

#include "Task.h"


namespace rts {

    template <typename T>
    concept ThreadPool = requires(T t, size_t num_threads, size_t queue_capacity, Task task) {
        {T(num_threads, queue_capacity)} noexcept;
        { t.init() } noexcept;
        { t.finalize() } noexcept;
        { t.enqueue(std::move(task)) } noexcept;
    };
}