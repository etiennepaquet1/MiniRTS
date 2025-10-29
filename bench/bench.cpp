#include <benchmark/benchmark.h>

#include "Runtime.h"
#include "Future.h"
#include "Promise.h"
#include "DefaultThreadPool.h"



// Measures the latency of enqueuing 1 million empty tasks with enqueue()
// (e.g. the time between enqueuing the first task and finishing the final task.
static void BM_Enqueue_Latency_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));
    constexpr int LOOP = 1'000'000;

    for (auto _ : state) {
        state.PauseTiming();

        // Initialize runtime with current configuration
        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        // Enqueue async tasks
        for (int i = 0; i < LOOP; ++i) {
            rts::enqueue([]{});
        }

        rts::finalize_soft();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;


        // Record metrics
        state.counters["Threads"]       = num_threads;
        state.counters["QueueCapacity"] = queue_capacity;
        state.counters["ns_per_task"]   = elapsed.count() / LOOP;
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;
    }
}

// Register combinations of (num_threads, queue_capacity)
BENCHMARK(BM_Enqueue_Latency_1_000_000)
    ->Args({1, 64})
    ->Args({1, 512})
    ->Args({1, 1 << 10})
    ->Args({1, 1 << 12})
    ->Args({1, 1 << 14})
    ->Args({1, 1 << 16})
    ->Args({1, 1 << 18})
    ->Args({1, 1 << 20})
    ->Args({2, 64})
    ->Args({2, 512})
    ->Args({2, 1 << 10})
    ->Args({2, 1 << 12})
    ->Args({2, 1 << 14})
    ->Args({2, 1 << 16})
    ->Args({2, 1 << 18})
    ->Args({2, 1 << 20})
    ->Args({3, 64})
    ->Args({3, 512})
    ->Args({3, 1 << 10})
    ->Args({3, 1 << 12})
    ->Args({3, 1 << 14})
    ->Args({3, 1 << 16})
    ->Args({3, 1 << 18})
    ->Args({3, 1 << 20})
    ->Args({4, 64})
    ->Args({4, 512})
    ->Args({4, 1 << 10})
    ->Args({4, 1 << 12})
    ->Args({4, 1 << 14})
    ->Args({4, 1 << 16})
    ->Args({4, 1 << 18})
    ->Args({4, 1 << 20})
    ->Unit(benchmark::kMillisecond);


// Measures the latency of enqueuing 1 million empty tasks with enqueue_async()
// (e.g. the time between enqueuing the first task and finishing the final task.
static void BM_Async_Latency_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));
    constexpr int LOOP = 1'000'000;

    for (auto _ : state) {
        state.PauseTiming();

        // Initialize runtime with current configuration
        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        // Enqueue async tasks
        for (int i = 0; i < LOOP; ++i) {
            rts::enqueue_async([]{});
        }

        rts::finalize_soft();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;


        // Record metrics
        state.counters["Threads"]       = num_threads;
        state.counters["QueueCapacity"] = queue_capacity;
        state.counters["ns_per_task"]   = elapsed.count() / LOOP;
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;
    }
}

// Register combinations of (num_threads, queue_capacity)
BENCHMARK(BM_Async_Latency_1_000_000)
    ->Args({1, 64})
    ->Args({1, 512})
    ->Args({1, 1 << 10})
    ->Args({1, 1 << 12})
    ->Args({1, 1 << 14})
    ->Args({1, 1 << 16})
    ->Args({1, 1 << 18})
    ->Args({1, 1 << 20})
    ->Args({2, 64})
    ->Args({2, 512})
    ->Args({2, 1 << 10})
    ->Args({2, 1 << 12})
    ->Args({2, 1 << 14})
    ->Args({2, 1 << 16})
    ->Args({2, 1 << 18})
    ->Args({2, 1 << 20})
    ->Args({3, 64})
    ->Args({3, 512})
    ->Args({3, 1 << 10})
    ->Args({3, 1 << 12})
    ->Args({3, 1 << 14})
    ->Args({3, 1 << 16})
    ->Args({3, 1 << 18})
    ->Args({3, 1 << 20})
    ->Args({4, 64})
    ->Args({4, 512})
    ->Args({4, 1 << 10})
    ->Args({4, 1 << 12})
    ->Args({4, 1 << 14})
    ->Args({4, 1 << 16})
    ->Args({4, 1 << 18})
    ->Args({4, 1 << 20})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
