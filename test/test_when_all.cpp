#include <gtest/gtest.h>


#include "api.h"
#include "utils.h"


TEST(ThreadPoolTests, TestWhenAll) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
    }) << "initialize_runtime() should not throw.";

    rts::async::Future<std::tuple<int>> tup = rts::async::when_all(
        rts::async::spawn([] { return 1; })
    );
    tup.then([](auto v){ std::cout << std::get<0>(v) << std::endl; });

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}

TEST(ThreadPoolTests, TestWhenAllMixed) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> called = 0;

    auto tup = rts::async::when_all(
        rts::async::spawn([] { return 42; }),
        rts::async::spawn([&] { debug_print() << "void future\n"; called.fetch_add(1, std::memory_order_relaxed); }),
        rts::async::spawn([] { return std::string("MiniRTS"); })
    );

    tup.then([&](auto results) {
        // Unpack the tuple
        const auto& [val1, _, val2] = results;
        debug_print() << "Mixed tuple values: " << val1 << ", " << val2 << "\n";
        EXPECT_EQ(val1, 42);
        EXPECT_EQ(val2, "MiniRTS");
        EXPECT_EQ(called.load(std::memory_order_relaxed), 1);
    });

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}


TEST(ThreadPoolTests, TestWhenAllVoidFuture) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
    }) << "initialize_runtime() should not throw.";

    auto tup = rts::async::when_all(
        rts::async::spawn([] {
            debug_print() << "1";
        })
    );

    tup.then([]{ debug_print() << "2"; });

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}