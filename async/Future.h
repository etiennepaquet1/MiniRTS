/**
 * @file Future.h
 * @brief The future type for all tasks.
 */

#pragma once

#include <exception>
#include <memory>
#include <mutex>
#include <syncstream>
#include <type_traits>
#include <utility>

#include "SharedState.h"
#include "Task.h"
#include "Utils.h"

namespace rts {
    /**
     * @brief Schedules a Task for asynchronous execution within the runtime system.
     */
    void enqueue(Task &&) noexcept;
}

namespace rts::async {

    template<typename T>
    class Promise;

    /**
     * @brief A Future represents a value that may not yet be available.
     *        It provides blocking retrieval via get(), readiness testing,
     *        and continuation chaining through then().
     *
     * @tparam T The type of value produced by the associated Promise.
     */
    template<typename T>
    class Future {
        std::shared_ptr<SharedState<T>> state_;

    public:
        using value_type = T;

        explicit Future(std::shared_ptr<SharedState<T>> s)
            : state_(std::move(s)) {
            assert(state_ && "Future constructed with null SharedState");
        }

        /**
         * @brief Returns true if the value or exception is already available.
         */
        [[nodiscard]] bool is_ready() const noexcept {
            assert(state_ && "is_ready() called on invalid Future");
            return state_->ready.load(std::memory_order_acquire);
        }

        /**
         * @brief Busy-waits until the Future is ready.
         *        (Consider replacing with condition_variable later.)
         */
        void wait() const {
            assert(state_ && "wait() called on invalid Future");
            while (!is_ready()) {
                // spin
            }
        }

        /**
         * @brief Blocks until ready, then returns the stored value or throws.
         */
        T get() {
            assert(state_ && "get() called on invalid Future");
            wait();
            if (state_->exception)
                std::rethrow_exception(state_->exception);

            if constexpr (std::is_void_v<T>) {
                return;
            } else {
                assert(state_->value.has_value() && "Future::get() called but no value set");
                return std::move(*state_->value);
            }
        }

        /**
         * @brief Detaches from the shared state, discarding this handle.
         */
        void detach() {
            assert(state_ && "detach() called on invalid Future");
            state_.reset();
        }

        /**
         * @brief Chains a continuation that executes once this Future is ready.
         *
         * @tparam F Callable type. Must accept a `T` argument.
         * @param f  The continuation function.
         * @return A new Future representing the result of `f`.
         */
        template<typename F>
        auto then(F &&f) -> Future<std::invoke_result_t<F, T>> {
            assert(state_ && "then() called on invalid Future");

            using U = std::invoke_result_t<F, T>;
            Promise<U> p;
            auto fut_next = p.get_future();
            assert(fut_next.is_ready() == false && "then() returned already-ready future (unexpected)");

            auto cont = [s = state_, func = std::forward<F>(f), p = std::move(p)]() mutable {
                assert(s && "Continuation invoked with null SharedState");
                try {
                    if (s->exception)
                        std::rethrow_exception(s->exception);

                    assert(s->value.has_value() && "Continuation called before value was set");

                    // Copy or move the stored value into the continuation.
                    auto val = s->value.value();

                    if constexpr (std::is_void_v<U>) {
                        func(val);
                        p.set_value();
                    } else {
                        p.set_value(func(val));
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

        // Forward declaration of void-specialized then()
        template<typename F>
        Future<std::invoke_result_t<F>> then(F &&f);
    };


    /**
     * @brief Specialization of Future<void>::then() for chaining continuations
     *        after a void-returning Future.
     */
    template<>
    template<typename F>
    Future<std::invoke_result_t<F>> Future<void>::then(F &&f) {
        assert(state_ && "then() called on invalid Future<void>");
        using U = std::invoke_result_t<F>;

        Promise<U> p;
        auto fut_next = p.get_future();

        auto cont = [s = state_, func = std::forward<F>(f), p = std::move(p)]() mutable {
            assert(s && "Continuation invoked with null SharedState<void>");
            try {
                if (s->exception)
                    std::rethrow_exception(s->exception);

                if constexpr (std::is_void_v<U>) {
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

} // namespace rts::async
