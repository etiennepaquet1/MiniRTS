#pragma once

#include <memory>
#include <mutex>
#include "SharedState.h"

namespace rts::async {

    // Fwd declaration to avoid circular dependency
    template<typename T>
        class Promise;
    template<typename T>
    class Future {
        std::shared_ptr<SharedState<T>> state_;

    public:
        explicit Future(std::shared_ptr<SharedState<T>> s) : state_(std::move(s)) {}

        bool is_ready() const noexcept {
            return state_->ready.load(std::memory_order_acquire);
        }

        void wait() const {
            if (!is_ready()) {
                std::unique_lock lk(state_->mtx);
                state_->cv.wait(lk, [&]{ return state_->ready.load(); });
            }
        }

        T get() {
            wait();
            if (state_->exception) std::rethrow_exception(state_->exception);
            if constexpr (std::is_void_v<T>) {
                return;
            } else {
                return std::move(*state_->value);
            }
        }

        template <typename F>
        auto then(F&& f) -> Future<std::invoke_result_t<F, T>> {
            using U = std::invoke_result_t<F, T>;
            Promise<U> p;
            auto fut_next = p.get_future();

            auto cont = [s = state_, func = std::forward<F>(f), p = std::move(p)]() mutable {
                try {
                    auto val = s->value.value();
                    if constexpr(std::is_void_v<U>) {
                        func(val);
                        p.set_value();
                    } else {
                        p.set_value(func(val));
                    }
                } catch (...) {
                    p.set_exception(std::current_exception());
                }
            };
            // if already ready, run immediately
            if (is_ready()) {
                enqueue(std::move(cont));
            } else {
                // otherwise register continuation
                {
                    std::lock_guard lk(state_->mtx);
                    state_->continuations.push_back(std::move(cont));
                }
            }
            return fut_next;
        }
    };
}