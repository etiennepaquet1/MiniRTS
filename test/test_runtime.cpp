#include <gtest/gtest.h>

#include "Future.h"
#include "Promise.h"
#include "Runtime.h"
#include "SharedState.h"
#include "Utils.h"
#include "DefaultThreadPool.h"

TEST(ThreadPoolTests, InitAndFinalize) {
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>();
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed(0);
    for (int i = 0; i < 100; i++) {
        rts::enqueue([i, &completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i));
            ++completed;
        });
    }
    while (completed < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_NO_THROW({
        rts::finalize();
    }) << "finalize() should not throw.";
}

TEST(ThreadPoolTests, enqueue_10_000) {
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(4);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed(0);
    for (int i = 0; i < 1000000; ++i) {
        rts::enqueue([&completed] {
            ++completed;
        });
    }
    while (completed < 1000000) {}

    EXPECT_NO_THROW({
        rts::finalize();
    }) << "finalize() should not throw.";
}

TEST(ThreadPoolTests, test_async) {
    pin_to_core(5);
    constexpr int LOOP = 1000000;

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1, 1 << 20);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> func_counter(0);
    std::atomic<int> cont_counter(0);

    for (int i = 0; i < LOOP; i++)
    {
        auto fut = rts::enqueue_async([&func_counter] {++func_counter;});
        fut.then([&cont_counter] {++cont_counter;});
    }
    while (func_counter < LOOP || cont_counter < LOOP) {}

    EXPECT_NO_THROW({
        rts::finalize();
    }) << "finalize() should not throw.";

    std::cout << "func_counter: " << func_counter << std::endl;
    std::cout << "cont_counter: " << cont_counter << std::endl;
}
