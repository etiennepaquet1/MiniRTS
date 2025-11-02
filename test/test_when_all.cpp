#include <gtest/gtest.h>

#include "future.h"
#include "promise.h"
#include "runtime.h"
#include "utils.h"
#include "default_thread_pool.h"


TEST(ThreadPoolTests, TestWhenAll) {
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime(1, 64);
    }) << "initialize_runtime() should not throw.";

    core::async::Future<std::tuple<int>> tup = rts::when_all(
        rts::async([] { return 1; })
    );
    tup.then([](auto v){ std::cout << std::get<0>(v) << std::endl; });

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}


// TEST(ThreadPoolTests, TestWhenAllVoidFuture) {
//     pin_to_core(5);
//
//     EXPECT_NO_THROW({
//         rts::initialize_runtime<rts::DefaultThreadPool>(1, 64);
//     }) << "initialize_runtime() should not throw.";
//
//     auto tup = rts::when_all(
//         rts::async([] {
//             debug_print() << "1";
//         })
//     );
//
//     tup.then([]{ debug_print() << "2"; });
//
//     EXPECT_NO_THROW({
//         rts::finalize_soft();
//     }) << "finalize_soft() should not throw.";
// }
