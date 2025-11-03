#pragma once


#include <cmath>
#include <limits>

#include "constants.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #include <immintrin.h> // _mm_pause on x86/x64
#endif

#if defined(_WIN32)
  #define NOMINMAX
  #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
  #include <pthread.h>
  #include <sched.h>
#endif


inline void pause_hint() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    _mm_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("yield");
#else
    std::this_thread::yield();
#endif
#elif defined(__arm__) || defined(_M_ARM)
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("yield");
#else
    std::this_thread::yield();
#endif
#else
    // Unknown arch: portable fallback
    std::this_thread::yield();
#endif
}

#if defined(__cpp_lib_syncstream) && (__cpp_lib_syncstream >= 201803L)
#include <syncstream>

inline auto debug_print() {
    if constexpr (core::DEBUG) {
        return std::osyncstream(std::cout) << "[Thread ID: "
            << std::this_thread::get_id() << "]: ";
    } else {
        static std::ostream null_stream{nullptr};
        return std::osyncstream(null_stream);
    }
}
#else
#include <mutex>
#include <sstream>

// Null buffer that discards all output
struct NullBuffer : public std::streambuf {
    int overflow(int c) override { return c; }
};

// Null output stream using NullBuffer
inline std::ostream& null_stream() {
    static NullBuffer nb;
    static std::ostream os(&nb);
    return os;
}

inline std::ostream& debug_print() {
    if constexpr (rts::core::DEBUG) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "[Thread ID: " << std::this_thread::get_id() << "]: ";
        return std::cout;
    } else {
        return null_stream();
    }
}
#endif


inline void pin_to_core(std::size_t core_id) {
#if defined(_WIN32)
    // Windows: Set affinity mask for the current thread
    HANDLE th = GetCurrentThread();
    // On Windows, affinity mask is a bitset; limit to 64 bits here.
    const DWORD_PTR mask = (core_id >= (sizeof(DWORD_PTR) * 8))
                           ? 0
                           : (DWORD_PTR(1) << core_id);
    if (mask == 0 || SetThreadAffinityMask(th, mask) == 0) {
        debug_print() << "Warning: SetThreadAffinityMask failed or core_id out of range";
    }
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    // POSIX: pthread_setaffinity_np (note: not available on macOS)
    // On macOS, pthread_setaffinity_np is absent; we treat as no-op.
#if defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    // CPU_SET expects an int index
    if (core_id <= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        CPU_SET(static_cast<int>(core_id), &cpuset);
        pthread_t current = pthread_self();
        int rc = pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            debug_print() << "Warning: pthread_setaffinity_np failed";
        }
    } else {
        debug_print() << "Warning: core_id too large for CPU_SET";
    }
#else
    // Other POSIX (e.g., macOS): no standard CPU affinity API
    debug_print() << "Info: CPU pinning not supported on this POSIX platform; no-op";
#endif
#else
    // Unknown platform: no-op
    (void)core_id;
    debug_print() << "Info: CPU pinning not supported on this platform; no-op";
#endif
}


inline void apply_backpressure(double saturation) noexcept {
    if (saturation < 0.5) return;

    const double factor = std::pow(saturation, 4.0);

    int spin_count = static_cast<int>(factor * 50'000);
    if (spin_count < 0) spin_count = 0;

    // Spin phase with architecture-specific pause/yield hint
    for (int i = 0; i < spin_count; ++i) {
        pause_hint();
    }
}
