#include <gtest/gtest.h>

#include "Runtime.h"
#include "ThreadPool.h"

constexpr size_t TEST_QUEUE_SIZE = 64;

TEST(ThreadPoolTests, InitAndFinalize) {
    // Create a ThreadPool instance with the template parameter.
    rts::Runtime<TEST_QUEUE_SIZE> pool(2);

    // Initialize with 4 threads.
    EXPECT_NO_THROW({
        pool.init();
    }) << "ThreadPool::init() should not throw.";

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    // Finalize (join threads).
    EXPECT_NO_THROW({
        pool.finalize();
    }) << "ThreadPool::finalize() should not throw.";
}
