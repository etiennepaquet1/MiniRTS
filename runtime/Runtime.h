#pragma once

#include <atomic>
#include <cassert>
#include <thread>
#include <memory>

#include "Task.h"
#include "ThreadPool.h"
#include "Constants.h"

namespace rts::async {
    template <typename T>
    class Promise;
    template <typename T>
    class Future;
}

namespace rts {

    class DefaultThreadPool;

    inline static std::atomic<bool> running{false};
    inline static void* active_thread_pool = nullptr;

    // Function pointer to enqueue() implementation.
    inline static void (*enqueue_fn)(Task &&) = nullptr;
    inline static void (*finalize_fn)(ShutdownMode mode) = nullptr;

    // Represents how saturated the worker queues are
    inline float saturation_cached = 0;
    // Worker queue capacity (Used for calculating saturation)

    template <ThreadPool T = DefaultThreadPool>
    bool initialize_runtime(size_t num_threads = std::thread::hardware_concurrency(),
                            size_t queue_capacity = kDefaultCapacity) noexcept {
        bool expected = false;
        if (running.compare_exchange_strong(expected, true,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            auto* pool = new T(num_threads, queue_capacity);
            pool->init();

            active_thread_pool = pool;
            enqueue_fn = [](Task &&task) noexcept {
                assert(task);
                static_cast<T*>(active_thread_pool)->enqueue(std::move(task));
            };
            finalize_fn = [](ShutdownMode mode) noexcept {
                auto* p = static_cast<T*>(active_thread_pool);
                if (!p) return;
                p->finalize(mode);
                delete p;
                active_thread_pool = nullptr;
                running.store(false, std::memory_order_release);
            };
            return true;
        }
        return false;
    }

    inline void enqueue(Task &&task) noexcept {
        assert(task);
        enqueue_fn(std::move(task));
    }

    inline void finalize_hard() noexcept {
        finalize_fn(HARD_SHUTDOWN);
    }
    inline void finalize_soft() noexcept {
        finalize_fn(SOFT_SHUTDOWN);
    }

    template<typename F, typename... Args>
    auto enqueue_async(F&& f, Args&&... args) -> async::Future<std::invoke_result_t<F, Args...>> {
        using T = std::invoke_result_t<F, Args...>;

        async::Promise<T> p;
        auto fut = p.get_future();

        // Capture the promise by value (moved)
        Task task = [func = std::forward<F>(f),
                     args_tuple = std::make_tuple(std::forward<Args>(args)...),
                     p = std::move(p)]() mutable {
            try {
                if constexpr (std::is_void_v<T>) {
                    std::apply(func, std::move(args_tuple));
                    p.set_value();
                } else {
                    T result = std::apply(func, std::move(args_tuple));
                    p.set_value(std::move(result));
                }
            } catch (...) {
                p.set_exception(std::current_exception());
            }
        };
        enqueue(std::move(task));
        return fut;
    }


    // TODO: Add exception path
    template <typename... Futures>
    auto when_all(Futures&&... futures) {
        using ResultTuple = std::tuple<typename std::decay_t<Futures>::value_type...>;
        using StateTuple  = std::tuple<std::optional<typename std::decay_t<Futures>::value_type>...>;

        async::Promise<ResultTuple> prom;
        auto out = prom.get_future();

        constexpr std::size_t N = sizeof...(Futures);
        if constexpr (N == 0) {
            prom.set_value(ResultTuple{});      // when() of nothing → ready empty tuple
            return out;
        }

        auto state     = std::make_shared<StateTuple>();
        auto remaining = std::make_shared<std::atomic<std::size_t>>(N);

        // Called after each input future completes; when the last one finishes,
        // we move the values out of the optionals into the final ResultTuple.
        auto fulfill = [prom = std::move(prom), state, remaining]() mutable {
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // All done: build the final tuple by moving each engaged optional.
                auto result = std::apply(
                    [](auto&... opts) -> ResultTuple {
                        // Precondition: each optional is engaged.
                        return ResultTuple{ std::move(*opts)... };
                    },
                    *state
                );
                prom.set_value(std::move(result));
            }
        };

        // Attach one continuation with a compile-time index I
        auto attach_one = [state, fulfill](auto& fut, [[maybe_unused]] auto index_c) {
            constexpr std::size_t I = decltype(index_c)::value;
            fut.then([state, fulfill](auto&& v) mutable {
                std::get<I>(*state).emplace(std::forward<decltype(v)>(v));
                fulfill();
            });
        };

        // Generate indices [0..N) and attach continuations
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (attach_one(futures, std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Futures...>{});

        return out;
    }


    // template<typename... Futures>
    // auto when_all(Futures &&... futures) {
    //     using ResultTuple = std::tuple<
    //         std::conditional_t<
    //             std::is_void_v<typename std::decay_t<Futures>::value_type>,
    //             std::monostate,
    //             typename std::decay_t<Futures>::value_type
    //         >...
    //     >;
    //
    //     using StateTuple = std::tuple<
    //         std::optional<
    //             std::conditional_t<
    //                 std::is_void_v<typename std::decay_t<Futures>::value_type>,
    //                 std::monostate,
    //                 typename std::decay_t<Futures>::value_type
    //             >
    //         >...
    //     >;
    //
    //     async::Promise<ResultTuple> prom;
    //     auto out = prom.get_future();
    //
    //     constexpr std::size_t N = sizeof...(Futures);
    //     if constexpr (N == 0) {
    //         prom.set_value(ResultTuple{});
    //         return out;
    //     }
    //
    //     auto state = std::make_shared<StateTuple>();
    //     auto remaining = std::make_shared<std::atomic<std::size_t> >(N);
    //
    //     // Called when one of the futures completes
    //     auto fulfill = [prom = std::move(prom), state, remaining]() mutable {
    //         if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
    //             auto result = std::apply(
    //                 [](auto &... opts) -> ResultTuple {
    //                     return ResultTuple{std::move(*opts)...};
    //                 },
    //                 *state
    //             );
    //             prom.set_value(std::move(result));
    //         }
    //     };
    //
    //     // Attach one continuation per input future
    //     auto attach_one = [state, fulfill](auto &fut, [[maybe_unused]] auto index_c) {
    //         constexpr std::size_t I = decltype(index_c)::value;
    //
    //         using FutT = typename std::decay_t<decltype(fut)>::value_type;
    //
    //         if constexpr (std::is_void_v<FutT>) {
    //             fut.then([state, fulfill]() mutable {
    //                 std::get<I>(*state).emplace(std::monostate{});
    //                 fulfill();
    //             });
    //         } else {
    //             fut.then([state, fulfill](auto &&v) mutable {
    //                 std::get<I>(*state).emplace(std::forward<decltype(v)>(v));
    //                 fulfill();
    //             });
    //         }
    //     };
    //
    //     // Generate indices [0..N) and attach continuations
    //     [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    //         (attach_one(futures, std::integral_constant<std::size_t, Is>{}), ...);
    //     }(std::index_sequence_for<Futures...>{});
    //
    //     return out;
    // }

} // namespace rts


