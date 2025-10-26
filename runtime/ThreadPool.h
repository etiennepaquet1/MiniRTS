#pragma once

#include "Constants.h"
#include "Task.h"


namespace rts {

    template <typename T>
    concept ThreadPool = requires(T t,
                                  size_t num_threads,
                                  size_t queue_capacity, const Task &task,
                                  ShutdownMode shutdown_mode)
    {
        { t.workers_ };
        { T(num_threads, queue_capacity) } noexcept;
        { t.init() } noexcept;
        { t.finalize(shutdown_mode) } noexcept;
        { t.enqueue(task) } noexcept;
    };
}