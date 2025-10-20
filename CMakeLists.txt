cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# For GCC and Clang
add_compile_options(-Wno-interference-size)

include(FetchContent)

# ─────────────────────────────────────────────
# Core library
# ─────────────────────────────────────────────
add_library(MiniRTS_lib
        runtime/ThreadPool.cpp
        runtime/Worker.cpp
        runtime/Task.cpp
        runtime/Runtime.cpp
)

target_include_directories(MiniRTS_lib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/runtime
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/WorkStealingQueue/include
)

# ─────────────────────────────────────────────
# Main executable
# ─────────────────────────────────────────────
#add_executable(MiniRTS
#        main.cpp       # create this file if not present
#)
#
#target_link_libraries(MiniRTS PRIVATE MiniRTS_lib)

# ─────────────────────────────────────────────
# Optional: GoogleTest
# ─────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(MiniRTS_tests
            test/test_threadpool.cpp
    )

    target_link_libraries(MiniRTS_tests
            PRIVATE
            gtest_main
            MiniRTS_lib
    )

    include(GoogleTest)
    gtest_discover_tests(MiniRTS_tests)
endif()

# ─────────────────────────────────────────────
# Optional: Google Benchmark
# ─────────────────────────────────────────────
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

if(BUILD_BENCHMARKS)
    FetchContent_Declare(
            benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

#    add_executable(MiniRTS_bench
#            benchmarks/bench_task.cpp
#            benchmarks/bench_threadpool.cpp
#    )
#
#    target_link_libraries(MiniRTS_bench
#            PRIVATE
#            benchmark::benchmark
#            MiniRTS_lib
#    )
endif()
