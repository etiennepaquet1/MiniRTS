#include <gtest/gtest.h>

#include "Runtime.h"
#include "ThreadPool.h"

constexpr size_t TEST_QUEUE_SIZE = 64;

TEST(ThreadPoolTests, InitAndFinalize) {
    // Create a ThreadPool instance with the template parameter.


    // Initialize with 4 threads.
    EXPECT_NO_THROW({
        rts::initialize_runtime<>();
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed {0};
    for (int i = 0; i < 500; i++) {
        rts::enqueue([i, &completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i));
            std::osyncstream(std::cout) <<
                "[Task Completed] : Slept for " << i << " ms"
                    << std::endl;
            ++completed;
        });
    }
    while (completed < 500) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}
