#pragma once

#include <arpa/inet.h>
#include <sched.h>
#include <iostream>

#include "Constants.h"

inline void pin_to_core(size_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    int rc = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
}

inline void debug_print(const char* msg) {
    if constexpr (rts::DEBUG)
    {    std::osyncstream(std::cout) << "[Thread ID: "
            << std::this_thread::get_id() << "]: "
                << msg << std::endl;
    }
}