#include <benchmark/benchmark.h>
#include <syncstream>
#include <iostream>

#include "runtime.h"
#include "future.h"
#include "promise.h"
#include "default_thread_pool.h"
#include "bench_utils.h"



// // Measures the latency of enqueuing 1 million empty tasks with enqueue()
// // (e.g. the time between enqueuing the first task and finishing the final task.)
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
            rts::enqueue([] {});
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


// Measures the overhead of enqueuing 1 million small wait tasks with enqueue()
// (e.g. the time between enqueuing the first task and finishing the final task
// minus the total processing time of the tasks.)

// The overhead of enqueueing can be calculated as
// OVERHEAD = TOTAL_TIME - ((TARGET_NS * LOOP) / NUM_CORES)
static void BM_Enqueue_Overhead_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    if (!is_tsc_invariant())
        state.SkipWithError("Invariant TSC required");

    constexpr int LOOP = 1'000'000;
    constexpr int TARGET_NS = 1000;

    int reps = calibrate_busy_work(TARGET_NS);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));

    for (auto _ : state) {
        state.PauseTiming();

        // Initialize runtime with current configuration
        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        // Enqueue tasks
        for (int i = 0; i < LOOP; ++i) {
            rts::enqueue([reps] {
                int iter {reps};
                benchmark::DoNotOptimize(iter);
                for (int i = 0; i < iter; ++i) {
                    benchmark::ClobberMemory();
                }
            });
        }

        rts::finalize_soft();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;


        // Record metrics
        state.counters["Threads"]       = num_threads;
        state.counters["QueueCapacity"] = queue_capacity;
        state.counters["overhead_ns_per_task"] = (elapsed.count() / LOOP) - (TARGET_NS / num_threads);
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;
    }
}

BENCHMARK(BM_Enqueue_Overhead_1_000_000)
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
// (e.g. the time between enqueuing the first task and finishing the final task.)
static void BM_Async_Latency_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));
    constexpr int LOOP = 1'000'000;

    for (auto _ : state) {
        state.PauseTiming();

        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < LOOP; ++i) {
            rts::enqueue_async([]{});
        }

        rts::finalize_soft();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;


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

// Measures the overhead of enqueuing 1 million small wait tasks with enqueue_async()
// (e.g. the time between enqueuing the first task and finishing the final task
// minus the total processing time of the tasks.)

// The overhead of enqueueing can be calculated as
// OVERHEAD = TOTAL_TIME - ((TARGET_NS * LOOP) / NUM_CORES)
static void BM_Async_Overhead_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    if (!is_tsc_invariant())
        state.SkipWithError("Invariant TSC required");

    constexpr int LOOP = 1'000'000;
    constexpr int TARGET_NS = 1000;

    int reps = calibrate_busy_work(TARGET_NS);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));

    for (auto _ : state) {
        state.PauseTiming();

        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < LOOP; ++i) {
            rts::enqueue_async([reps] {
                int iter {reps};
                benchmark::DoNotOptimize(iter);
                for (int i = 0; i < iter; ++i) {
                    benchmark::ClobberMemory();
                }
            });
        }

        rts::finalize_soft();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;

        state.counters["Threads"]       = num_threads;
        state.counters["QueueCapacity"] = queue_capacity;
        state.counters["overhead_ns_per_task"] = (elapsed.count() / LOOP) - (TARGET_NS / num_threads);
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;
    }
}

// Register combinations of (num_threads, queue_capacity)
BENCHMARK(BM_Async_Overhead_1_000_000)
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


// Measures the latency of executing a long sequential .then() chain.
// The benchmark creates a Future and chains N continuations on it.
// We measure the time from the start of the chain until the final .get() completes.
static void BM_Then_Chain_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));
    constexpr int LOOP        = 1'000'000;

    for (auto _ : state) {
        state.PauseTiming();

        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();

        auto fut = rts::enqueue_async([] {});
        for (int i = 0; i < LOOP; ++i)
            fut = fut.then([] {});

        fut.get();

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;

        state.PauseTiming();
        rts::finalize_soft();
        state.ResumeTiming();

        // Record metrics
        state.counters["Threads"]         = num_threads;
        state.counters["QueueCapacity"]   = queue_capacity;
        state.counters["ChainLength"]     = LOOP;
        state.counters["ns_per_then"]     = elapsed.count() / LOOP;
        state.counters["Total_ns"]        = elapsed.count();
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;

    }
}

// Register combinations of (num_threads, queue_capacity, chain_length)
BENCHMARK(BM_Then_Chain_1_000_000)
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


// Measures the direct cost of attaching a continuation via .then()
// Excludes task creation and Promise overhead.
static void BM_Then_Registration_1_000_000(benchmark::State &state) {
    pin_to_core(5);

    const auto num_threads    = static_cast<size_t>(state.range(0));
    const auto queue_capacity = static_cast<size_t>(state.range(1));
    constexpr int LOOP = 1'000'000;

    for (auto _ : state) {
        state.PauseTiming();

        // Initialize runtime with current configuration
        rts::initialize_runtime<rts::DefaultThreadPool>(num_threads, queue_capacity);

        // Prepare a pre-built future so that only .then() is timed
        std::vector<rts::async::Future<void>> futures;
        futures.reserve(LOOP);
        for (int i = 0; i < LOOP; ++i) {
            rts::async::Promise<void> p;
            futures.push_back(p.get_future());
        }

        state.ResumeTiming();

        auto start = std::chrono::steady_clock::now();
        for (auto &f : futures) {
            f.then([] {});
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double, std::nano> elapsed = end - start;

        state.PauseTiming();
        rts::finalize_soft();
        state.ResumeTiming();

        // Record metrics
        state.counters["Threads"]         = num_threads;
        state.counters["QueueCapacity"]   = queue_capacity;
        state.counters["ns_per_then"]     = elapsed.count() / LOOP;
        state.counters["Throughput_Mops"] = (LOOP / elapsed.count()) * 1e3;
    }
}

// Register combinations of (num_threads, queue_capacity)
BENCHMARK(BM_Then_Registration_1_000_000)
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
