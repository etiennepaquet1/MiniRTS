

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
auto when_any(Futures... futures) {
    constexpr std::size_t N = sizeof...(Futures);
    static_assert(N > 0, "when_any() requires at least one Future");

    constexpr bool all_void = (std::is_void_v<future_value_t<Futures>> && ...);

    if constexpr (all_void) {
        // ── All inputs are Future<void> → return Future<void>
        Promise<void> prom;
        auto out = prom.get_future();

        auto remaining = std::make_shared<std::atomic<std::size_t>>(1);
        auto fulfill = [prom = std::move(prom), remaining]() mutable {
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1)
                prom.set_value();
        };

        auto attach_one = [fulfill](auto& fut) {
            fut.then([fulfill]() mutable { fulfill(); });
        };
        (attach_one(futures), ...);

        return out; // return here, no trailing return afterward
    } else {
        // ── Mixed / non-void case → Future<std::variant<...>>
        using variant_t = std::variant<
            std::conditional_t<std::is_void_v<future_value_t<Futures>>,
                               std::monostate,
                               future_value_t<Futures>>...
        >;

        Promise<variant_t> prom;
        auto out = prom.get_future();

        // A flag to ensure only the first result is taken
        auto won = std::make_shared<std::atomic<bool>>(false);

        auto fulfill = [prom = std::move(prom), won](auto&& result) mutable {
            bool expected = false;
            if (won->compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                prom.set_value(std::forward<decltype(result)>(result));
            }
        };

        auto attach_one = [fulfill, won]<std::size_t I>(auto& fut, std::integral_constant<std::size_t, I>) {
            using V = future_value_t<decltype(fut)>;

            if constexpr (std::is_void_v<V>) {
                fut.then([fulfill, won]() mutable {
                    if (!won->load(std::memory_order_acquire)) {
                        fulfill(std::variant_alternative_t<I, variant_t>{std::monostate{}});
                    }
                });
            } else {
                fut.then([fulfill, won](auto&& v) mutable {
                    if (!won->load(std::memory_order_acquire)) {
                        fulfill(std::variant_alternative_t<I, variant_t>{std::forward<decltype(v)>(v)});
                    }
                });
            }
        };

        // Expand the futures over their indices
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (attach_one(futures, std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Futures...>{});

        return out;
    }
}

}