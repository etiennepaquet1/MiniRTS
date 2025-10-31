# ─────────────────────────────────────────────
# Google Benchmark
# ─────────────────────────────────────────────
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

if(BUILD_BENCHMARKS)
    include(FetchContent)
    FetchContent_Declare(
            benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_LTO ON CACHE BOOL "" FORCE)

    # Build benchmark in Release mode
    set(_OLD_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE Release)
    FetchContent_MakeAvailable(benchmark)
    set(CMAKE_BUILD_TYPE ${_OLD_BUILD_TYPE})

    # Bench helper interface
    add_library(Bench INTERFACE)
    target_include_directories(Bench INTERFACE ${CMAKE_SOURCE_DIR}/bench)

    # Benchmark executable
    add_executable(MiniRTS_bench bench.cpp)

    target_link_libraries(MiniRTS_bench
            PRIVATE
            benchmark::benchmark
            Runtime
            Bench
    )
endif()
