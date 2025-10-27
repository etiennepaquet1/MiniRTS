#include <gtest/gtest.h>

#include "Future.h"
#include "Promise.h"
#include "Runtime.h"
#include "Utils.h"
#include "DefaultThreadPool.h"

TEST(ThreadPoolTests, InitAndFinalize) {
    pin_to_core(5);
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>();
    }) << "initialize_runtime() should not throw.";

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>();
    }) << "initialize_runtime() should not throw.";

    EXPECT_NO_THROW({
        rts::finalize_hard();
    }) << "finalize_hard() should not throw.";
}
TEST(ThreadPoolTests, BasicEnqueues) {
    pin_to_core(5);
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>();
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed(0);
    for (int i = 0; i < 100; i++) {
        rts::enqueue([&completed]() {
            ++completed;
        });
    }
    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    ASSERT_EQ(completed.load(), 100);
}



TEST(ThreadPoolTests, enqueue_1000_000) {
    pin_to_core(5);
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(4);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < 1000000; ++i) {
        rts::enqueue([] {});
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize() should not throw.";

}

TEST(ThreadPoolTests, test_then) {
    pin_to_core(5);
    constexpr int LOOP = 1000000;

    rts::direct_counter = 0;

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1, 1024);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < LOOP; i++)
    {
        auto fut = rts::enqueue_async([] {});
        fut.then([] {});
    }

    std::cout << "Direct Counter: " << rts::direct_counter << std::endl;

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize() should not throw.";
}

TEST(ThreadPoolTests, test_multiple_then) {
    pin_to_core(5);
    constexpr int LOOP = 1000000;

    rts::direct_counter = 0;

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1, 1024);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < LOOP; i++)
    {
        auto fut = rts::enqueue_async([] {});
        fut.then([] {});
        fut.then([] {});
        fut.then([] {});
        fut.then([] {});
    }

    std::cout << "Direct Counter: " << rts::direct_counter << std::endl;

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize() should not throw.";
}

TEST(ThreadPoolTests, test_multiple_then_2_cores) {
    pin_to_core(5);
    constexpr int LOOP = 1000000;

    rts::direct_counter = 0;

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(2, 1024);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < LOOP; i++)
    {
        auto fut = rts::enqueue_async([] {});
        fut.then([] {});
        fut.then([] {});
        fut.then([] {});
        fut.then([] {});
    }

    std::cout << "Direct Counter: " << rts::direct_counter << std::endl;

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize() should not throw.";
}
