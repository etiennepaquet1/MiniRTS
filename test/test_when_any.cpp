#include <gtest/gtest.h>


#include "api.h"
#include "utils.h"
#include "when_any.h"


TEST(ThreadPoolTests, TestWhenAnySingle) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
        }) << "initialize_runtime() should not throw.";

    auto fut = rts::async::when_any(
        rts::async::spawn([] { return 123; })
    );

    fut.then([](auto var) {
        std::visit([](auto &&v) {
            debug_print() << "First result: " << v;
            EXPECT_EQ(v, 123);
        }, var);
    });

    EXPECT_NO_THROW({
        rts::finalize_soft();
        }) << "finalize_soft() should not throw.";
}

TEST(ThreadPoolTests, TestWhenAnyMixed) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
        }) << "initialize_runtime() should not throw.";

    std::atomic<int> side_effects = 0;

    auto fut = rts::async::when_any(
        rts::async::spawn([] { return 42; }),
        rts::async::spawn([&] { side_effects.fetch_add(1, std::memory_order_relaxed); }),
        rts::async::spawn([] { return std::string("Hello"); })
    );

    fut.then([](auto var) {
        std::visit([](auto &&v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                debug_print() << "void alternative\n";
            } else {
                debug_print() << v << "\n";
            }
        }, var);
    });
    // Each test run should only see the first finished value
    EXPECT_LE(side_effects.load(std::memory_order_relaxed), 1);

    EXPECT_NO_THROW({
        rts::finalize_soft();
        }) << "finalize_soft() should not throw.";
}


TEST(ThreadPoolTests, TestWhenAnyVoidOnly) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
        }) << "initialize_runtime() should not throw.";

    std::atomic<int> called = 0;

    auto f = rts::async::when_any(
        rts::async::spawn([&] {
            debug_print() << "task 1\n";
            called.fetch_add(1, std::memory_order_relaxed);
        }),
        rts::async::spawn([&] {
            debug_print() << "task 2\n";
            called.fetch_add(1, std::memory_order_relaxed);
        }),
        rts::async::spawn([&] {
            debug_print() << "task 3\n";
            called.fetch_add(1, std::memory_order_relaxed);
        })
    );

    f.then([&] {
        debug_print() << "Winner\n";
        EXPECT_EQ(called.load(std::memory_order_relaxed), 1);
    });

    EXPECT_NO_THROW({
        rts::finalize_soft();
        }) << "finalize_soft() should not throw.";
}
