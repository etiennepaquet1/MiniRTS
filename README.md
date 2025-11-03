Welcome to MiniRTS!

A work-stealing task scheduling library designed for fine-grained, latency-sensitive parallelism in single-producer workloads.


MiniRTS is a performance-oriented C++23 task runtime that implements a lightweight work-stealing scheduler and a continuation-based async framework.
It allows developers to launch millions of fine-grained tasks with sub-microsecond scheduling overhead, using a per-core work-stealing deque design inspired by research runtimes like libfork and HPX.
The runtime provides Future/Promise primitives with support for continuation chaining (then()), composition (when_all() / when_any()).
MiniRTS is designed for applications where scheduling overhead and latency must be minimized. It uses NUMA-friendly work-stealing by keeping continuations local to their producing worker, minimizing cross-core contention.

MiniRTS presents a simple and user-friendly API to allow users to easily keep track of the dependencies of their tasks. Scheduling a task is as easy as
rts::enqueue([]{ task(); });
If you'd like to check out more, please read the tour of MiniRTS and try it on your own system!


#include "api.h"

core::async::Future<int> fibonacci(int n) {
    if (n <= 1) {
        // Base case: wrap simple return in a Future
        return rts::async::spawn([n] { return n; });
    }

    auto f1 = fibonacci(n - 1); // spawn child tasks
    auto f2 = fibonacci(n - 2);

    // Combine results with when_all() and chain with .then()
    return rts::async::when_all(std::move(f1), std::move(f2))
        .then([](std::tuple<int, int> results) {
            auto [a, b] = results;
            return a + b;
        });
}


How does MiniRTS work?



Performance
MiniRTS was designed to offer low-latency and low-overhead task scheduling. Under the bench/ folder you can find a benchmarking suite that benchmark various operations, 
like the latency of scheduling and executing a million empty tasks, the overhead of enqueuing a task of predetermined length and the latency of chaining a million .then() continuations.
The MiniRTS benchmarks are run with a selected list of combination of three parameters: number of tasks, size of SPSC queue and number of workers. This gives us a good idea of how well MiniRTS handles various workloads

Please read the benchmark test descriptions if you're interested.
There's a sample result of my benchmarks that I ran on my device with chrt -r 99 and isolated cores. Here's a sample of this 1-million task latency result:

    BM_Enqueue_Latency_1_000_000/1/64              176 ms          172 ms            4 QueueCapacity=64 Threads=1 Throughput_Mops=6.86724 ns_per_task=145.619
    BM_Enqueue_Latency_1_000_000/1/256             144 ms          143 ms            5 QueueCapacity=256 Threads=1 Throughput_Mops=7.03875 ns_per_task=142.071
    BM_Enqueue_Latency_1_000_000/1/1024            141 ms          141 ms            5 QueueCapacity=1.024k Threads=1 Throughput_Mops=7.03468 ns_per_task=142.153
    BM_Enqueue_Latency_1_000_000/1/4096            136 ms          136 ms            5 QueueCapacity=4.096k Threads=1 Throughput_Mops=7.56674 ns_per_task=132.157
    BM_Enqueue_Latency_1_000_000/1/16384           131 ms          130 ms            5 QueueCapacity=16.384k Threads=1 Throughput_Mops=7.76566 ns_per_task=128.772
    BM_Enqueue_Latency_1_000_000/1/65536           134 ms          134 ms            5 QueueCapacity=65.536k Threads=1 Throughput_Mops=7.44495 ns_per_task=134.319
    BM_Enqueue_Latency_1_000_000/1/262144          137 ms          137 ms            5 QueueCapacity=262.144k Threads=1 Throughput_Mops=7.17327 ns_per_task=139.406
    BM_Enqueue_Latency_1_000_000/1/1048576         148 ms          148 ms            5 QueueCapacity=1.04858M Threads=1 Throughput_Mops=6.74625 ns_per_task=148.23

Setup:
    Ubuntu 24.04.3 LTS, 
    Kernel version 6.14.0-33-generic
    Intel i5-8400@3.80GHz
    g++ 13.3.0





What's next?
In the near future I aim to introduce a few new features that could help performance and ease of use:

- Implement HPX-style task_blocks for scoped parallelism
- Look deeper into custom allocators (The library currently uses new/delete on task creation and destruction. Custom allocators can reduce that overhead.
- Look into false sharing optimizations for Worker and Thread Pool objects. The queues already use hardware_destructuve_interference properly.
- Research benchmarking in more depth and try to find better ways to benchmark MiniRTS, including adding benchmarks for more cores.

