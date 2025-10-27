#pragma once

#include <memory>
#include <mutex>

#include "SharedState.h"
#include "Future.h"
#include "Worker.h"

namespace rts {
    inline std::atomic<int> direct_counter {};
}
namespace rts::async {

    template<typename T>
    class Promise {
        std::shared_ptr<SharedState<T>> state_;

    public:
        Promise() noexcept : state_(std::make_shared<SharedState<T>>()) {}

        Future<T> get_future() noexcept { return Future<T>(state_); }

        template<typename U = T>
        void set_value(U&& value) noexcept requires (!std::is_void_v<U>) {
            {
                std::lock_guard lk(state_->mtx);
                state_->value = std::move(value);
                state_->ready.store(true, std::memory_order_release);
            }
            for (auto& cont : state_->continuations)
                rts::enqueue(std::move(cont));
        }

        template<typename U = T>
        void set_value() noexcept requires std::is_void_v<U> {
            {
                std::lock_guard lk(state_->mtx);
                state_->ready.store(true, std::memory_order_release);
                for (auto& cont : state_->continuations) {
                    assert(tls_worker);
                    if (!tls_worker->enqueue_local(cont)) {
                        // No space in WSQ: Execute it directly
                        cont();
                        ++direct_counter;
                    }
                }
            }
        }

        void set_exception(std::exception_ptr e) noexcept {
            state_->exception = e;
            state_->ready.store(true, std::memory_order_release);
            for (auto& cont : state_->continuations)
                rts::enqueue(std::move(cont));
        }
    };
}
