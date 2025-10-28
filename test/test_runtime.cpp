#include <gtest/gtest.h>

#include "Future.h"
#include "Promise.h"
#include "Runtime.h"
#include "Utils.h"
#include "DefaultThreadPool.h"


// ─────────────────────────────────────────────────────────────
// ---------------------  Integration Tests  -------------------
// ─────────────────────────────────────────────────────────────

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

TEST(ThreadPoolTests, TestWorkStealing){
    pin_to_core(5);
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(2, 1024);
    }) << "initialize_runtime() should not throw.";

    for (int i = 0; i < 1000; i++) {
        rts::enqueue([i]{std::cout << i << std::endl;});
        rts::enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::osyncstream(std::cout) << "---" << i << std::endl;
        });
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}


TEST(ThreadPoolTests, TestEmptyFunctions) {
    pin_to_core(5);

    constexpr int LOOP {1};

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1, 64);
    }) << "initialize_runtime() should not throw.";

    for (size_t i = 0; i < LOOP; ++i) {
        rts::enqueue([] {
            std::osyncstream(std::cout) << "function called" << std::endl;
        });
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}

TEST(ThreadPoolTests, TestIncrement) {
    pin_to_core(5);

    constexpr int LOOP {10};

    std::atomic<int> count {0};
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(1, 64);
    }) << "initialize_runtime() should not throw.";

    for (size_t i = 0; i < LOOP; ++i) {
        rts::enqueue([&count] {++count;});
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    EXPECT_EQ(count, LOOP);
}


// ─────────────────────────────────────────────────────────────
// -------------------  Parameterized Tests  -------------------
// ─────────────────────────────────────────────────────────────

struct EnqueueParams {
    size_t num_threads;
    size_t queue_capacity;
    size_t loop_count;
};

std::vector<EnqueueParams> GenerateParams() {
    std::vector<EnqueueParams> params;
    std::vector<size_t> cores = {1, 2, 3, 4};
    std::vector<size_t> caps = {64, 256, 1024, 4096, 1 << 14, 1 << 16, 1 << 18, 1 << 20};
    std::vector<size_t> loop_counts = {100, 1'000, 10'000, 100'000, 1'000'000};

    for (auto num_threads : cores) {
        for (auto cap : caps) {
            for (auto loop : loop_counts) {
                params.push_back({num_threads, cap, loop});
            }
        }
    }
    return params;
}

class EmptyEnqueueTests : public ::testing::TestWithParam<EnqueueParams> {};
TEST_P(EmptyEnqueueTests, EnqueueStress) {
    const auto& p = GetParam();
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(p.num_threads, p.queue_capacity);
    }) << "initialize_runtime() should not throw.";

    for (size_t i = 0; i < p.loop_count; ++i) {
        rts::enqueue([] {});
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}
INSTANTIATE_TEST_SUITE_P(
    EmptyEnqueueTests,
    EmptyEnqueueTests,
    ::testing::ValuesIn(GenerateParams()),
    [](const testing::TestParamInfo<EnqueueParams>& info) {
        std::ostringstream os;
        os << "Cores" << info.param.num_threads
           << "_Cap" << info.param.queue_capacity
           << "_Loop" << info.param.loop_count;
        return os.str();
    });


class IncrementTaskTests : public ::testing::TestWithParam<EnqueueParams> {};
TEST_P(IncrementTaskTests, EnqueueStress) {
    const auto& p = GetParam();
    pin_to_core(5);
    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(p.num_threads, p.queue_capacity);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> completed(0);
    for (size_t i = 0; i < p.loop_count; ++i) {
        rts::enqueue([&completed] {
            ++completed;
        });
    }
    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    ASSERT_EQ(completed.load(), p.loop_count);
}
INSTANTIATE_TEST_SUITE_P(
    IncrementTaskTests,
    IncrementTaskTests,
    ::testing::ValuesIn(GenerateParams()),
    [](const testing::TestParamInfo<EnqueueParams>& info) {
        std::ostringstream os;
        os << "Cores" << info.param.num_threads
           << "_Cap" << info.param.queue_capacity
           << "_Loop" << info.param.loop_count;
        return os.str();
    });


class ThenParamTests : public ::testing::TestWithParam<EnqueueParams> {};

TEST_P(ThenParamTests, ContinuationStress) {
    const auto& p = GetParam();
    pin_to_core(5);


    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(p.num_threads, p.queue_capacity);
    }) << "initialize_runtime() should not throw.";

    for (size_t i = 0; i < p.loop_count; ++i) {
        auto fut = rts::enqueue_async([] {});
        fut.then([] {});
    }

    std::cout << "[Cores " << p.num_threads
              << " | Cap " << p.queue_capacity
              << " | Loop " << p.loop_count
              << "]" << std::endl;

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";
}

// Instantiate the parameter combinations
INSTANTIATE_TEST_SUITE_P(
    ThreadPoolThenTests,
    ThenParamTests,
    ::testing::ValuesIn(GenerateParams()),
    [](const testing::TestParamInfo<EnqueueParams>& info) {
        std::ostringstream os;
        os << "Cores" << info.param.num_threads
           << "_Cap" << info.param.queue_capacity
           << "_Loop" << info.param.loop_count;
        return os.str();
    });


class MultipleThenTests : public ::testing::TestWithParam<EnqueueParams> {};
TEST_P(MultipleThenTests, MultipleThenStress) {
    const auto& p = GetParam();
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(p.num_threads, p.queue_capacity);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> counter{0};
    for (size_t i = 0; i < p.loop_count; ++i) {
        auto fut = rts::enqueue_async([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
        fut.then([&counter] { ++counter; });
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    ASSERT_EQ(counter.load(), static_cast<int>(p.loop_count * 10));
}

INSTANTIATE_TEST_SUITE_P(
    MultipleThenTests,
    MultipleThenTests,
    ::testing::ValuesIn(GenerateParams()),
    [](const testing::TestParamInfo<EnqueueParams>& info) {
        std::ostringstream os;
        os << "Cores" << info.param.num_threads
           << "_Cap" << info.param.queue_capacity
           << "_Loop" << info.param.loop_count;
        return os.str();
    });

class RecursiveThenTests : public ::testing::TestWithParam<EnqueueParams> {};
TEST_P(RecursiveThenTests, MultipleThenStress) {
    const auto& p = GetParam();
    pin_to_core(5);

    EXPECT_NO_THROW({
        rts::initialize_runtime<rts::DefaultThreadPool>(p.num_threads, p.queue_capacity);
    }) << "initialize_runtime() should not throw.";

    std::atomic<int> counter{0};
    for (size_t i = 0; i < p.loop_count; ++i) {
        auto f1 = rts::enqueue_async([&counter] { ++counter; });
        auto f2 = f1.then([&counter] { ++counter; });
        auto f3 = f1.then([&counter] { ++counter; });
        auto f4 = f3.then([&counter] { ++counter; });
        auto f5 = f3.then([&counter] { ++counter; });
        auto f6 = f5.then([&counter] { ++counter; });
        auto f7 = f5.then([&counter] { ++counter; });
        auto f8 = f7.then([&counter] { ++counter; });
        auto f9 = f7.then([&counter] { ++counter; });
    }

    EXPECT_NO_THROW({
        rts::finalize_soft();
    }) << "finalize_soft() should not throw.";

    ASSERT_EQ(counter.load(), static_cast<int>(p.loop_count * 9));
}

INSTANTIATE_TEST_SUITE_P(
    RecursiveThenTests,
    RecursiveThenTests,
    ::testing::ValuesIn(GenerateParams()),
    [](const testing::TestParamInfo<EnqueueParams>& info) {
        std::ostringstream os;
        os << "Cores" << info.param.num_threads
           << "_Cap" << info.param.queue_capacity
           << "_Loop" << info.param.loop_count;
        return os.str();
    });
