#pragma once

#include <cstddef>


namespace rts {
    constexpr bool DEBUG = false;

#ifdef __cpp_lib_hardware_interference_size
    inline constexpr size_t kCacheLine = std::hardware_destructive_interference_size;
#else
    inline constexpr size_t kCacheLine = 64;
#endif
    inline constexpr size_t kDefaultCapacity = 1024;


    enum ShutdownMode {
        HARD_SHUTDOWN = 1,
        SOFT_SHUTDOWN = 2
    };

}