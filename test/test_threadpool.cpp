#include <gtest/gtest.h>

#include "Runtime.h"
#include "ThreadPool.h"
#include "Utils.h"

TEST(ThreadPoolTests, InitAndFinalize) {
    EXPECT_NO_THROW({
        rts::initialize_runtime<>();
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed(0);
    for (int i = 0; i < 100; i++) {
        rts::enqueue([i, &completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i));
            std::osyncstream(std::cout) <<
                "[Task Completed] : Slept for " << i << " ms"
                    << std::endl;
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
        rts::initialize_runtime<>(4);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < 1000000; ++i) {
        rts::enqueue([] {});
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NO_THROW({
        rts::finalize();
    }) << "finalize() should not throw.";
}
