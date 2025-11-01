/*

    Welcome to MiniRTS, an (almost) lock-free, low-overhead, low-latency runtime system!


    MiniRTS provides an easy-to-use API to configure your runtime, create and schedule tasks and
    attach continuations to existing tasks. Let's start by instantiating your runtime.

    To use MiniRTS, you simply need to #include "api/api.h". This will import all existing user-facing facilities.

*/

#include "api.h"


int main() {
    rts::initialize_runtime();

    rts::enqueue([]{});

    rts::finalize_soft();
}




/*
int main() {


// To instantiate a new runtime, simply call rts::instantiate_runtime().
// By default this will spawn a DefaultThreadPool with as many workers as the # of cores on your hardware.

    rts::initialize_runtime();


// Our thread pool and its workers are now ready. Let's start by enqueuing a simple operation.
// Use rts::enqueue() for independent tasks.

    rts::enqueue([] { std::cout << "Hello from Worker" << std::endl; });


// If we need the result of the task, we will need to use Futures.
// Simply declare an async::Future of the type you want returned.

    core::async::Future<int> f1 = rts::async([] {
        return 3141592;
    });

// And wait for it using Future.get().
    std::cout << f1.get() << std::endl;


// Awesome. But what if you want to use that result as the input for another operation?
// The best way to do this is to use Future.then()

    core::async::Future<int> f2 = rts::async([] {
        return 299'792'458;
    });


// The lambda's function signature has to be the same as the Future's return type.

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


// You can also register multiple continuations on the same Future:

    auto f6 = rts::async([] {});

    f6.then([] {});
    f6.then([] {});


// You can also wait for multiple Futures to complete before continuing.
// Use rts::when_all() to combine several Futures into one composite Future.

// This is useful when you have multiple async operations that can run in parallel
// but whose results are needed together at a later point.

auto f7 = rts::async([] {
    return 21;
});

auto f8 = rts::async([] {
    return 2;
});

auto f9 = rts::async([] {
    return 1;
});

// rts::when_all() returns a Future containing a std::tuple of all results.

auto all = rts::when_all(std::move(f7), std::move(f8), std::move(f9));

// We can now attach a continuation that takes the tuple as input:

auto f10 = all.then([](std::tuple<int, int, int> results) {
    auto [a, b, c] = results;
    return a * b + c;
});

// Or simply block until the combined result is ready:

std::cout << f10.get() << std::endl;


// Done! Once you're finished submitting tasks, don't forget to shut down the runtime.
// Use finalize_soft() for a graceful shutdown where the workers are allowed to finish the work
// in their queues, or finalize_hard() to stop the workers immediately.

rts::finalize_soft();


    return 0;
}
*/






