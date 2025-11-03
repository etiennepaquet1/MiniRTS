#include <gtest/gtest.h>

#include "future.h"
#include "promise.h"
#include "runtime.h"
#include "utils.h"
#include "default_thread_pool.h"
#include "when_all.h"


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


// TEST(ThreadPoolTests, TestWhenAllVoidFuture) {
//     pin_to_core(5);
//
//     EXPECT_NO_THROW({
//         rts::initialize_runtime<rts::DefaultThreadPool>(1, 64);
//     }) << "initialize_runtime() should not throw.";
//
//     auto tup = async::when_all(
//         async::spawn([] {
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
