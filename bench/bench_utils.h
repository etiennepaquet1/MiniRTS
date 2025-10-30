#pragma once

#if defined(_MSC_VER)
  #include <intrin.h>
  #define GET_CPUID(info, x) __cpuid(info, x)
#else
  #include <cpuid.h>
  #define GET_CPUID(info, x) __get_cpuid(x, &info[0], &info[1], &info[2], &info[3])
#endif


inline bool is_tsc_invariant() {
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(0x80000007, &eax, &ebx, &ecx, &edx))
        return false;
    return edx & (1 << 8); // Bit 8 of EDX means invariant TSC
}


// Calculates how many iterations of a busy loop amount to target_ns.
inline int calibrate_busy_work(int target_ns = 1000) {
    int iter = 1'000'000'000;
    auto start = std::chrono::high_resolution_clock::now();

    benchmark::DoNotOptimize(iter);
    for (int i = 0; i < iter; ++i) {
        benchmark::ClobberMemory();
    }
    auto end = std::chrono::high_resolution_clock::now();

    double total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double time_per_iter_ns = total_ns / static_cast<double>(iter);
    int iterations_per_target = static_cast<int>(target_ns / time_per_iter_ns);

    if constexpr(rts::DEBUG) {
        std::osyncstream(std::cout)
       << "Total: " << total_ns / 1e6 << " ms, "
       << "Per iteration: " << time_per_iter_ns << " ns, "
       << "Iterations per " << target_ns << " ns: " << iterations_per_target << "\n";
    }
    return iterations_per_target;
}
