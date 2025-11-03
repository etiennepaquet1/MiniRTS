/**
 * @file when_all.h
 * @brief Combines multiple Futures into one Future that resolves when all inputs are ready.
 */

#pragma once

#include <tuple>
#include <utility>
#include <atomic>
#include <exception>
#include <variant>
#include "future.h"
#include "promise.h"


namespace rts::async {

template <typename F>
using future_value_t = typename std::decay_t<F>::value_type;

template <typename... Futures>
auto when_all(Futures&&... futures) {
    constexpr std::size_t N = sizeof...(Futures);
    static_assert(N > 0, "when_all() requires at least one Future");

    constexpr bool all_void = (std::is_void_v<future_value_t<Futures>> && ...);

    if constexpr (all_void) {
        // ── All inputs are Future<void> → return Future<void>
        Promise<void> prom;
        auto out = prom.get_future();

        auto remaining = std::make_shared<std::atomic<std::size_t>>(N);
        auto fulfill = [prom = std::move(prom), remaining]() mutable {
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                prom.set_value();
            }
        };

        auto attach_one = [fulfill](auto& fut) {
            fut.then([fulfill]() mutable { fulfill(); });
        };
        (attach_one(futures), ...);

        return out; // return here, no trailing return afterward
    } else {
        // ── Mixed / non-void case → Future<std::tuple<...>> (void → monostate)
        using result_tuple_t = std::tuple<
            std::conditional_t<std::is_void_v<future_value_t<Futures>>,
                               std::monostate,
                               future_value_t<Futures>>...
        >;

        using state_tuple_t = std::tuple<
            std::conditional_t<std::is_void_v<future_value_t<Futures>>,
                               std::optional<std::monostate>,
                               std::optional<future_value_t<Futures>>>...
        >;

        Promise<result_tuple_t> prom;
        auto out = prom.get_future();

        auto state     = std::make_shared<state_tuple_t>();
        auto remaining = std::make_shared<std::atomic<std::size_t>>(N);

        auto fulfill = [prom = std::move(prom), state, remaining]() mutable {
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                auto result = std::apply(
                    []<typename... Ts>(std::optional<Ts>&... opts) -> result_tuple_t {
                        return result_tuple_t{ (opts ? std::move(*opts) : Ts{})... };
                    },
                    *state
                );
                prom.set_value(std::move(result));
            }
        };

        auto attach_one = [state, fulfill]<std::size_t I>(auto& fut, std::integral_constant<std::size_t, I>) {
            using V = future_value_t<decltype(fut)>;
            if constexpr (std::is_void_v<V>) {
                fut.then([state, fulfill]() mutable {
                    std::get<I>(*state).emplace(std::monostate{});
                    fulfill();
                });
            } else {
                fut.then([state, fulfill](auto&& v) mutable {
                    std::get<I>(*state).emplace(std::forward<decltype(v)>(v));
                    fulfill();
                });
            }
        };

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (attach_one(futures, std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Futures...>{});

        return out; // return here (different type), but only this branch is active
    }
}
} // namespace rts::async
