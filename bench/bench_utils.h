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

