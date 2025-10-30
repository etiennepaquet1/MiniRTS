/**
 * @file promise.h
 * @brief Provides the Promise side of the MiniRTS async system.
 *
 * A Promise<T> represents the *producer* of a value or exception
 * that a Future<T> will eventually observe. The Promise is responsible
 * for setting the value or exception and notifying any registered
 * continuations.
 */

#pragma once

#include <cassert>
#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>

#include "shared_state.h"
#include "future.h"
#include "worker.h"

namespace rts::async {

    /**
     * @brief Promise<T> acts as the producer counterpart to Future<T>.
     *
     * It provides functions to:
     *   - produce a result (`set_value()`),
     *   - signal an error (`set_exception()`), and
     *   - obtain the associated Future (`get_future()`).
     *
     * Promises are move-only and intended to be used within the RTS
     * task scheduling system, which ensures continuations are executed
     * either locally (on the same worker) or globally if local enqueuing fails.
     *
     * @tparam T The type of value that will be produced.
     */
    template<typename T>
    class Promise {
        std::shared_ptr<SharedState<T>> state_;  ///< Shared state between Promise and Future

    public:
        using value_type = T;

        /// @brief Constructs a new Promise with a fresh shared state.
        Promise() noexcept
            : state_(std::make_shared<SharedState<T>>()) {
            assert(state_ && "Promise must have valid SharedState");
        }

        Promise(Promise&& other) noexcept
            : state_(std::move(other.state_)) {}

        Promise& operator=(Promise&& other) noexcept {
            if (this != &other) {
                state_ = std::move(other.state_);
            }
            return *this;
        }

        Promise(const Promise&) = default;
        Promise& operator=(const Promise&) = default;

        /**
         * @brief Retrieves the associated Future.
         * @return A Future<T> that will be ready once set_value() or set_exception() is called.
         */
        [[nodiscard]] Future<T> get_future() noexcept {
            assert(state_ && "get_future() called on moved-from Promise");
            return Future<T>(state_);
        }

        /**
         * @brief Sets the result value (non-void case).
         *
         * @tparam U The type of the value being set (defaults to T).
         * @param value The value to store in the shared state.
         *
         * Notifies all registered continuations once ready.
         */
        template<typename U = T>
        void set_value(U&& value) noexcept requires (!std::is_void_v<U>) {
            assert(state_ && "set_value() called on moved-from Promise");

            {
                std::lock_guard lk(state_->mtx);
                // Prevent double fulfillment
                assert(!state_->ready.load(std::memory_order_acquire) && "set_value() called twice");
                state_->value = std::forward<U>(value);
                state_->ready.store(true, std::memory_order_release);
            }

            // Schedule all registered continuations
            for (auto& cont : state_->continuations) {
                assert(cont && "Continuation is invalid");
                assert(tls_worker && "tls_worker must be set for local enqueue");
                if (!tls_worker->enqueue_local(std::move(cont))) {
                    // Local WSQ full → run immediately
                    cont();
                    cont.destroy();
                }
            }
        }

        /**
         * @brief Marks the Promise as ready without a value (void specialization).
         */
        template<typename U = T>
        void set_value() noexcept requires std::is_void_v<U> {
            assert(state_ && "set_value() called on moved-from Promise");

            {
                std::lock_guard lk(state_->mtx);
                assert(!state_->ready.load(std::memory_order_acquire) && "set_value() called twice");
                state_->ready.store(true, std::memory_order_release);
            }

            for (auto& cont : state_->continuations) {
                assert(cont && "Continuation is invalid");
                assert(tls_worker && "tls_worker must be set for local enqueue");
                if (!tls_worker->enqueue_local(std::move(cont))) {
                    cont();
                    cont.destroy();
                }
            }
        }

        /**
         * @brief Sets an exception instead of a value, marking the Promise as failed.
         *
         * @param e The exception to propagate to the Future.
         */
        void set_exception(std::exception_ptr e) noexcept {
            assert(state_ && "set_exception() called on moved-from Promise");
            assert(e && "set_exception() called with null exception_ptr");

            {
                std::lock_guard lk(state_->mtx);
                assert(!state_->ready.load(std::memory_order_acquire) && "set_exception() called twice");
                state_->exception = std::move(e);
                state_->ready.store(true, std::memory_order_release);
            }

            for (auto& cont : state_->continuations) {
                assert(cont && "Continuation is invalid");
                rts::enqueue(std::move(cont));
            }
        }
    };

} // namespace rts::async
