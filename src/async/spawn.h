#pragma once

/**
 * @file spawn.h
 * @brief Provides the `spawn()` function to asynchronously run callables in the RTS.
 */


#include <cassert>
#include <tuple>
#include <utility>
#include <exception>
#include <type_traits>


#include "concepts.h"
#include "future.h"
#include "promise.h"


namespace rts::async {
    template <typename T>
    class Promise;

    template <typename T>
    requires core::concepts::FutureValue<T>
    class Future;

    /**
     * @brief Asynchronously enqueues a callable for execution and returns a Future for its result.
     *
     * @tparam F Callable type.
     * @tparam Args Argument pack for callable.
     * @return async::Future<T> representing the result.
     *
     * @note The callable is executed inside the runtime’s thread pool.
     */
    template<typename F, typename... Args>
    auto spawn(F&& f, Args&&... args)
        -> Future<std::invoke_result_t<F, Args...>> {

        using T = std::invoke_result_t<F, Args...>;

        assert(core::running.load(std::memory_order_acquire) && "enqueue_async() called on inactive runtime");
        assert(core::enqueue_fn && "enqueue_async() called before initialization");

        Promise<T> p;
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

        rts::enqueue(std::move(task));
        return fut;
    } // namespace async
}