#include <gtest/gtest.h>

#include "Future.h"
#include "Promise.h"
#include "Runtime.h"
#include "SharedState.h"
#include "Utils.h"
#include "DefaultThreadPool.h"

// TEST(ThreadPoolTests, InitAndFinalize) {
//     EXPECT_NO_THROW({
//         rts::initialize_runtime<rts::DefaultThreadPool>();
//     }) << "initialize_runtime() should not throw.";
//
//     std::atomic<int> completed(0);
//     for (int i = 0; i < 100; i++) {
//         rts::enqueue([i, &completed]() {
//             std::this_thread::sleep_for(std::chrono::milliseconds(i));
//             ++completed;
//         });
//     }
//     while (completed < 100) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
//     EXPECT_NO_THROW({
//         rts::finalize();
//     }) << "finalize() should not throw.";
// }
//
// TEST(ThreadPoolTests, enqueue_10_000) {
//     EXPECT_NO_THROW({
//         rts::initialize_runtime<rts::DefaultThreadPool>(4);
//     }) << "initialize_runtime() should not throw.";
//
//     std::atomic<int> completed(0);
//     for (int i = 0; i < 1000000; ++i) {
//         rts::enqueue([&completed] {
//             ++completed;
//         });
//     }
//     while (completed < 1000000) {}
//
//     EXPECT_NO_THROW({
//         rts::finalize();
//     }) << "finalize() should not throw.";
// }

TEST(ThreadPoolTests, test_async) {
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1);
    }) << "initialize_runtime() should not throw.";

    rts::SharedState<void> state;
    rts::Future<void> future(std::make_shared<rts::SharedState<void>>());
    rts::Promise<void> p;
    rts::async([] {});

    EXPECT_NO_THROW({
        rts::finalize();
    }) << "finalize() should not throw.";
}
