# Welcome to MiniRTS\!

A work-stealing task scheduling library designed for fine-grained, latency-sensitive parallelism in single-producer workloads.

## Overview

MiniRTS is a performance-oriented C++23 task runtime that implements a lightweight work-stealing scheduler and a continuation-based async framework. It allows developers to launch millions of fine-grained tasks with sub-microsecond scheduling overhead, using a per-core work-stealing deque design inspired by research runtimes like libfork and HPX.

The runtime provides Future/Promise primitives with support for continuation chaining (`then()`) and composition (`when_all() / when_any()`). MiniRTS is designed for applications where scheduling overhead and latency must be minimized. It uses NUMA-friendly work-stealing by keeping continuations local to their producing worker, minimizing cross-core contention.

MiniRTS presents a simple and user-friendly API to allow users to easily keep track of the dependencies of their tasks. Scheduling a task is as easy as:

```cpp
rts::enqueue([]{ task(); });
```

If you'd like to check out more, please read the tour of MiniRTS and try it on your own system\!

## API Example: Async Fibonacci

Here is a sample implementation of an asynchronous Fibonacci function using `spawn`, `when_all`, and `then` to manage dependencies.

```cpp
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
```

-----

## A Tour of MiniRTS

Here is a walkthrough of the core API features.

### 1\. Initialization

To instantiate a new runtime, simply call `rts::initialize_runtime()`. By default, this will spawn a `DefaultThreadPool` with as many workers as there are hardware cores.

```cpp
// To instantiate a new runtime, simply call rts::initialize_runtime().
// By default this will spawn a DefaultThreadPool with as many workers as the # of cores on your hardware.
rts::initialize_runtime();
```

### 2\. Enqueuing Simple Tasks

Our thread pool and its workers are now ready. Use `rts::enqueue()` for independent, "fire-and-forget" tasks that don't require a return value.

```cpp
// Use rts::enqueue() for independent tasks.
rts::enqueue([] { std::cout << "Hello from Worker" << std::endl; });
```

### 3\. Spawning Tasks with Futures

If you need to get a result back from a task, use `rts::async::spawn()`. This returns a `Future` of the specified type. You can block and wait for the result using `.get()`.

```cpp
// If we need the result of the task, we will need to use Futures.
// Simply declare a Future of the type you want returned.
rts::async::Future<int> f1 = rts::async::spawn([] {
    return 3141592;
});

// And wait for it using Future.get().
std::cout << f1.get() << std::endl;
```

### 4\. Chaining Continuations with .then()

The best way to use a Future's result is to attach a *continuation* task using `.then()`. This continuation will automatically execute once the Future is ready, receiving the result as an argument.

You can chain multiple `.then()` calls to create a pipeline of dependent operations.

```cpp
rts::async::Future<int> f2 = rts::async::spawn([] {
    return 299'792'458;
});

// The lambda's function signature has to match the Future's return type.
auto f3 = f2.then([](int x) {
    return x * 2.236936;
});

// You can chain continuations with .then() chains:
auto f4 = f3.then([](double x) {
    return x / 1.61803398875;
});

auto f5 = f4.then([](double x) {
    return x / 6.67430;
});
```

You can also register multiple independent continuations on the *same* Future.

```cpp
// You can also register multiple continuations on the same Future:
auto f6 = rts::async::spawn([] {});

f6.then([] {}); // This will run when f6 is ready
f6.then([] {}); // This will *also* run when f6 is ready
```

### 5\. Composing Futures with when\_all()

To wait for multiple Futures to complete before running a continuation, use `rts::when_all()`. This is perfect for "fan-out, fan-in" parallelism. It combines several Futures into a single new Future that holds a `std::tuple` of all the results.

```cpp
// This is useful when you have multiple spawn operations that can run in parallel
// but whose results are needed together at a later point.
auto f7 = rts::async::spawn([] {
    return 21;
});

auto f8 = rts::async::spawn([] {
    return 2;
});

auto f9 = rts::async::spawn([] {
    return 1;
});

// rts::when_all() returns a Future containing a std::tuple of all results.
auto all = rts::async::when_all(std::move(f7), std::move(f8), std::move(f9));

// We can now attach a continuation that takes the tuple as input:
auto f10 = all.then([](std::tuple<int, int, int> results) {
    auto [a, b, c] = results;
    return a * b + c;
});

// Or simply block until the combined result is ready:
std::cout << f10.get() << std::endl;
```

### 6\. Exception Propagation

MiniRTS propagates exceptions thrown inside tasks through their Futures. You can catch these exceptions by wrapping `.get()` in a `try/catch` block.

```cpp
// Let's throw an exception from a task:
auto f_ex = rts::async::spawn([]() -> int {
    throw std::runtime_error("boom");
    return 0;
});

// If we block until the result using get(), the exception will be rethrown here:
try {
    f_ex.get();
    std::cout << "This will not be printed!" << std::endl;
} catch (const std::exception& e) {
    std::cout << "Caught exception from task: " << e.what() << std::endl;
}
```

> **Important:** If a Future enters an exceptional state, its `.then()` continuations **will not** execute. The exception is propagated directly to the final Future in the chain.

```cpp
// Note: Your continuation WILL NOT execute if the Future is in an exceptional state.
auto f_bad = rts::async::spawn([]() -> int { throw std::runtime_error("kaboom"); })
    .then([](int) {
        std::cout << "This continuation should not run!" << std::endl;
        return -1;
    });

try {
    f_bad.get();
} catch (const std::exception& e) {
    std::cout << "Continuation not executed; caught exception: " << e.what() << std::endl;
}
```

### 7\. Shutdown

Once you're finished submitting tasks, don't forget to shut down the runtime.

  * `rts::finalize_soft()`: A graceful shutdown. Workers will finish all tasks currently in their queues.
  * `rts::finalize_hard()`: Stops all workers immediately.

<!-- end list -->

```cpp
rts::finalize_soft();
```

-----

## Performance

MiniRTS was designed to offer low-latency and low-overhead task scheduling. Under the `bench/` folder, you can find a benchmarking suite that benchmarks various operations, like:

  * The latency of scheduling and executing a million empty tasks.
  * The overhead of enqueuing a task of predetermined length.
  * The latency of chaining a million `.then()` continuations.

The MiniRTS benchmarks are run with a selected list of combinations of three parameters: number of tasks, size of SPSC queue, and number of workers. This gives us a good idea of how well MiniRTS handles various workloads.

### Sample Result: 1-Million Task Latency

Here's a sample of benchmark results run with `chrt -r 99` and isolated cores.

**Setup:**

  * **OS:** Ubuntu 24.04.3 LTS
  * **Kernel:** 6.14.0-33-generic
  * **CPU:** Intel i5-8400 @ 3.80GHz
  * **Compiler:** g++ 13.3.0

**Benchmark Output (`BM_Enqueue_Latency_1_000_000` / 1 Thread):**

```bash
BM_Enqueue_Latency_1_000_000/1/64           176 ms          172 ms          4 QueueCapacity=64 Threads=1 Throughput_Mops=6.86724 ns_per_task=145.619
BM_Enqueue_Latency_1_000_000/1/256          144 ms          143 ms          5 QueueCapacity=256 Threads=1 Throughput_Mops=7.03875 ns_per_task=142.071
BM_Enqueue_Latency_1_000_000/1/1024         141 ms          141 ms          5 QueueCapacity=1.024k Threads=1 Throughput_Mops=7.03468 ns_per_task=142.153
BM_Enqueue_Latency_1_000_000/1/4096         136 ms          136 ms          5 QueueCapacity=4.096k Threads=1 Throughput_Mops=7.56674 ns_per_task=132.157
BM_Enqueue_Latency_1_000_000/1/16384        131 ms          130 ms          5 QueueCapacity=16.384k Threads=1 Throughput_Mops=7.76566 ns_per_task=128.772
BM_Enqueue_Latency_1_000_000/1/65536        134 ms          134 ms          5 QueueCapacity=65.536k Threads=1 Throughput_Mops=7.44495 ns_per_task=134.319
BM_Enqueue_Latency_1_000_000/1/262144       137 ms          137 ms          5 QueueCapacity=262.144k Threads=1 Throughput_Mops=7.17327 ns_per_task=139.406
BM_Enqueue_Latency_1_000_000/1/1048576      148 ms          148 ms          5 QueueCapacity=1.04858M Threads=1 Throughput_Mops=6.74625 ns_per_task=148.23
```

-----
## How does MiniRTS work?
<img width="1300" height="700" alt="image" src="https://github.com/user-attachments/assets/49af2bc6-a995-4fb7-a6ec-15e828a42969" />

This diagram illustrates the core scheduling flow in the MiniRTS runtime system. User code submits a callable through rts::enqueue(task) or rts::async::spawn(task), which passes it to the central thread pool for distribution. 
The thread pool manages a collection of workers. Each worker continuously executes tasks from its own deque; if a worker’s queue becomes empty, it attempts to steal work from another worker’s deque. This decentralized, work-stealing model balances load dynamically across cores.

---

<img width="2048" height="1048" alt="image" src="https://github.com/user-attachments/assets/373ad3e2-4cd8-4101-9858-512933cad936" />

This diagram shows the internal design of each worker thread in the MiniRTS runtime system. Tasks submitted from the thread pool are first placed into a worker’s SPSC queue. When a worker's WSQ is empty, the worker drains its contents into its work-stealing deque (WSQ), the primary structure from which the worker consumes tasks. Each worker continuously pops tasks from the bottom of its own deque. When a worker’s SPSC queue becomes empty, it attempts to steal tasks from the top of another worker’s deque. Conversely, when continuations (e.g., .then() chains) are created, they are enqueued directly back into the same worker’s local WSQ to maintain NUMA locality and cache affinity.

-----

## What's Next? (Roadmap)

In the near future, I aim to introduce a few new features that could help performance and ease of use:

  * Implement HPX-style `task_blocks` for scoped parallelism.
  * Look deeper into custom allocators (The library currently uses `new`/`delete` on task creation and destruction. Custom allocators can reduce that overhead.)
  * Look into adding an inline buffer into Task objects for small callables
  * Look into false sharing optimizations for Worker and ThreadPool objects. (The queues already use `hardware_destructuve_interference` properly.)
  * Research benchmarking in more depth and try to find better ways to benchmark MiniRTS, including adding benchmarks for more cores.
