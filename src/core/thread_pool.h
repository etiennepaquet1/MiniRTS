#pragma once

#include "constants.h"
#include "task.h"


namespace rts {

    template <typename T>
    concept ThreadPool = requires(T t,
                                  size_t num_threads,
                                  size_t queue_capacity, Task task,
                                  ShutdownMode shutdown_mode)
    {
        { T(num_threads, queue_capacity) } noexcept;
        { t.init() } noexcept -> std::same_as<void>;
        { t.finalize(shutdown_mode) } noexcept -> std::same_as<void>;
        { t.enqueue(std::move(task)) } noexcept -> std::same_as<void>;
    };
}