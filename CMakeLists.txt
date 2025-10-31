cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# For GCC and Clang
add_compile_options(-Wno-interference-size)

include(FetchContent)

# ─────────────────────────────────────────────
# Core library
# ─────────────────────────────────────────────
add_library(Runtime
        src/core/worker.cpp
        src/core/concepts.h
)

target_include_directories(Runtime
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/async
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/WorkStealingQueue/include
        ${CMAKE_SOURCE_DIR}/external/MPMCQueue/include
)

add_library(Bench INTERFACE
        src/api/api.h)
target_include_directories(Bench INTERFACE ${CMAKE_SOURCE_DIR}/bench)



# ─────────────────────────────────────────────
# GoogleTest
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
            test/test_runtime.cpp
            test/test_future.cpp
            test/test_when_all.cpp
    )

    target_link_libraries(MiniRTS_tests
            PRIVATE
            gtest_main
            Runtime
    )

    include(GoogleTest)
    gtest_discover_tests(MiniRTS_tests)
endif()


# ─────────────────────────────────────────────
# Google Benchmark
# ─────────────────────────────────────────────
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

if(BUILD_BENCHMARKS)
    # Download Google Benchmark
    FetchContent_Declare(
            benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # Force benchmark to build in Release mode, even if the main project is Debug
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_LTO ON CACHE BOOL "" FORCE)
    set(CMAKE_BUILD_TYPE_BENCHMARK Release CACHE STRING "Build type for benchmark" FORCE)

    # Trick: temporarily override the build type for this subproject only
    set(_OLD_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE Release)
    FetchContent_MakeAvailable(benchmark)
    set(CMAKE_BUILD_TYPE ${_OLD_BUILD_TYPE})

    # Now build your benchmark executable
    add_executable(MiniRTS_bench
            bench/bench.cpp
    )

    target_include_directories(MiniRTS_bench
            PRIVATE
            ${CMAKE_SOURCE_DIR}/bench
    )

    target_link_libraries(MiniRTS_bench
            PRIVATE
            benchmark::benchmark
            Runtime
            Bench
    )
endif()
