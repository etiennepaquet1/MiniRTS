#include <gtest/gtest.h>

#include "api.h"
#include "utils.h"



TEST(ThreadPoolTests, TestReturnValue) {
    pin_to_core(5);

    constexpr int LOOP {1'000'000};

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
    }) << "initialize_runtime() should not throw.";

    for (size_t i = 0; i < LOOP; ++i) {
        rts::async::spawn([] { return 23; })
                .then([](int a) { EXPECT_EQ(a, 23); });
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}

TEST(ThreadPoolTests, TestVoidThen) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    std::atomic<int> called = 0;
    rts::async::spawn([] {}).then([&] { called.fetch_add(1, std::memory_order_relaxed); });

    rts::finalize_soft();
    EXPECT_EQ(called.load(), 1);
}

TEST(ThreadPoolTests, TestChainedThen) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    auto f = rts::async::spawn([] { return 2; })
        .then([](int x) { return x * 3; })
        .then([](int y) { return y + 4; })
        .then([](int z) { EXPECT_EQ(z, 10); });

    rts::finalize_soft();
}

TEST(ThreadPoolTests, TestExceptionPropagation) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    auto f = rts::async::spawn([]() -> int { throw std::runtime_error("boom"); return 0; })
        .then([](int) { FAIL() << "Continuation should not run"; });

    EXPECT_THROW(f.get(), std::runtime_error);

    rts::finalize_soft();
}

TEST(ThreadPoolTests, TestExceptionInThen) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    auto f = rts::async::spawn([] { return 42; })
        .then([](int) -> int { throw std::logic_error("oops"); });

    EXPECT_THROW(f.get(), std::logic_error);
    rts::finalize_soft();
}


TEST(ThreadPoolTests, TestThenOnReadyFuture) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    rts::async::Promise<int> p;
    auto f = p.get_future();
    p.set_value(99);

    std::atomic<int> result = 0;
    f.then([&](int v) { result.store(v, std::memory_order_relaxed); });

    rts::finalize_soft();
    EXPECT_EQ(result.load(), 99);
}


TEST(ThreadPoolTests, TestLongChain) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    constexpr int N = 1'000'000;
    auto fut = rts::async::spawn([] { return 1; });
    for (int i = 0; i < N; ++i) {
        fut = fut.then([](int x) { return x + 1; });
    }
    EXPECT_EQ(fut.get(), N + 1);

    rts::finalize_soft();
}


TEST(ThreadPoolTests, TestVoidChain) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    std::atomic<int> called = 0;
    rts::async::spawn([] {})
        .then([] {})      // void → void
        .then([&] { ++called; });

    rts::finalize_soft();
    EXPECT_EQ(called.load(), 1);
}

TEST(ThreadPoolTests, TestMultipleThenOnSameFuture) {
    pin_to_core(5);
    rts::initialize_runtime(1, 64);

    auto f = rts::async::spawn([] { return 10; });
    std::atomic<int> c1 = 0, c2 = 0;

    f.then([&](int v) { c1 = v; });
    f.then([&](int v) { c2 = v * 2; });

    rts::finalize_soft();
    EXPECT_EQ(c1.load(), 10);
    EXPECT_EQ(c2.load(), 20);
}
