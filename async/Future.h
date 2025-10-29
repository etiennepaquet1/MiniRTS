#pragma once

#include <memory>
#include <mutex>
#include <syncstream>

#include "SharedState.h"
#include "Task.h"
#include "Utils.h"

namespace rts {
    void enqueue(Task &&) noexcept;
}

namespace rts::async {

    template <typename T>
    class Promise;

    template<typename T>
    class Future {
        std::shared_ptr<SharedState<T>> state_;

    public:
        using value_type = T;

        Future(std::shared_ptr<SharedState<T>> s) : state_(std::move(s)) {}

        bool is_ready() const noexcept {
            return state_->ready.load(std::memory_order_acquire);
        }

        void wait() const {
            while (!is_ready()) {}
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

        void detach() {
            state_.reset();
        }

        template <typename F>
        auto then(F&& f) -> Future<std::invoke_result_t<F, T>> {
            using U = std::invoke_result_t<F, T>;
            Promise<U> p;
            auto fut_next = p.get_future();

            auto cont = [s = state_, func = std::forward<F>(f), p = std::move(p)]() mutable {
                try {
                    if (s->exception) {
                        std::rethrow_exception(s->exception);
                    }
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
            // If already ready, run immediately.
            if (is_ready()) {
                enqueue(std::move(cont));
            } else {
                // Otherwise, register continuation.
                {
                    std::lock_guard lk(state_->mtx);
                    state_->continuations.push_back(std::move(cont));
                }
            }
            return fut_next;
        }


        template<typename F>
        Future<std::invoke_result_t<F>> then(F &&f);
    };

    // Specialization for void Futures.
    template <>
    template <typename F>
    Future<std::invoke_result_t<F>> Future<void>::then(F&& f) {

        using U = std::invoke_result_t<F>;
        Promise<U> p;
        auto fut_next = p.get_future();

        auto cont = [s = state_, func = std::forward<F>(f), p = std::move(p)]() mutable {
            try {
                if (s->exception) {
                    std::rethrow_exception(s->exception);
                }
                if constexpr(std::is_void_v<U>) {
                    func();
                    p.set_value();
                } else {
                    p.set_value(func());
                }
            } catch (...) {
                p.set_exception(std::current_exception());
            }
        };
        {
            std::lock_guard lk(state_->mtx);
            if (is_ready()) {
                enqueue(std::move(cont));
            } else {
                state_->continuations.push_back(std::move(cont));
            }
        }
        return fut_next;
    }
}



