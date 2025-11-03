/**
* @file constants.h
 * @brief Configuration constants and shutdown modes for the MiniRTS runtime.
 *
 * This header defines cache-line alignment constants, default queue capacities,
 * debug flags, and shutdown mode options shared across the runtime system.
 */

#pragma once

#include <cstddef>
#include <thread>

namespace rts::core {

    /**
     * @brief Cache-line size used for padding and alignment.
     *
     * Uses `std::hardware_destructive_interference_size` when available,
     * otherwise defaults to 64 bytes.
     */
#ifdef __cpp_lib_hardware_interference_size
    inline constexpr size_t kCacheLine = std::hardware_destructive_interference_size;
#else
    inline constexpr size_t kCacheLine = 64;
#endif


    /**
     * @brief Default number of worker threads in the runtime system.
     *
     * Uses `std::thread::hardware_concurrency()` when available to query the number
     * of hardware threads on the system (e.g., logical CPU cores). Falls back to 1
     * if the standard thread library isn't available or `hardware_concurrency()` is not supported.
     */
#if defined(_GLIBCXX_HAS_GTHREADS) || defined(_LIBCPP_HAS_THREAD_API_PTHREAD) || defined(_MSC_VER)
    inline const std::size_t kDefaultWorkerCount =
        [] {
            const unsigned n = std::thread::hardware_concurrency();
            return n ? static_cast<std::size_t>(n) : 1u;
    }();
#else
    inline constexpr std::size_t kDefaultWorkerCount = 1;
#endif

    /**
     * @brief Default capacity for internal task or queue buffers.
     */
    inline constexpr size_t kDefaultCapacity = 1024;

    /**
     * @brief Global debug flag for conditional instrumentation and assertions.
     */
    constexpr bool DEBUG = false;

    /**
     * @brief Defines shutdown modes for the runtime system.
     *
     * - `HARD_SHUTDOWN`: Immediately stops all workers and tasks.
     * - `SOFT_SHUTDOWN`: Allows in-flight tasks to complete before stopping.
     */
    enum ShutdownMode {
        HARD_SHUTDOWN = 1,
        SOFT_SHUTDOWN = 2
    };
}
