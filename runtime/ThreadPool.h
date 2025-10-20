#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <functional>

#include "Worker.h"


namespace rts {

#ifdef __cpp_lib_hardware_interference_size
    inline constexpr std::size_t kCacheLine = std::hardware_destructive_interference_size;
#else
    inline constexpr std::size_t kCacheLine = 64;
#endif

    inline constexpr size_t kDefaultCapacity = 1024;

    template <typename T>
    concept ThreadPool = requires(T t, size_t num_threads, size_t queue_capacity, const Task& task) {
        {T(num_threads, queue_capacity)} noexcept;
        { t.init() } noexcept;
        { t.finalize() } noexcept;
        { t.enqueue(task) } noexcept;
    };

    class AnyThreadPool {
        std::shared_ptr<void> ptr_;

        std::function<void(const Task&)> enqueue_fn_;
        std::function<void()> init_fn_;
        std::function<void()> finalize_fn_;

    public:
        template<ThreadPool T>
        AnyThreadPool(std::unique_ptr<T> pool)
            : ptr_(std::move(pool)),
              enqueue_fn_([p = std::static_pointer_cast<T>(ptr_)](const Task& t){ p->enqueue(t); }),
              init_fn_([p = std::static_pointer_cast<T>(ptr_)]() { p->init(); }),
              finalize_fn_([p = std::static_pointer_cast<T>(ptr_)](){ p->finalize(); })
        {}
        void enqueue(const Task& t) { enqueue_fn_(t); }
        void init() { init_fn_(); }
        void finalize() { finalize_fn_(); }
    };


    class DefaultThreadPool {
        static constexpr size_t kDefaultThreadPoolCapacity = kDefaultCapacity;

        std::vector<Worker> workers_;
        size_t num_threads_;
        std::shared_ptr<std::atomic<bool>> stop_flag_;
        int round_robin_;
        size_t queue_capacity_;

    public:
        DefaultThreadPool(
            size_t num_threads = std::thread::hardware_concurrency(),
            size_t queue_capacity = kDefaultThreadPoolCapacity) noexcept :
                num_threads_(num_threads),
                stop_flag_(std::make_shared<std::atomic<bool> >(false)),
                round_robin_(0),
                queue_capacity_(queue_capacity) {}

        ~DefaultThreadPool() = default;

        void init() noexcept {
            workers_.reserve(num_threads_);
            for (int i = 0; i < num_threads_; i++) {
                workers_.emplace_back(i, stop_flag_, queue_capacity_);
                workers_.back().run();
            }
        }

        void finalize() noexcept {
            stop_flag_->store(true, std::memory_order_relaxed);
            for (auto& worker : workers_) {
                worker.join();
            }
        }

        void enqueue(const Task& task) noexcept {
            workers_[round_robin_++].enqueue(task);
            if (round_robin_ == num_threads_) {
                round_robin_ = 0;
            }
        }
    };

    static_assert(ThreadPool<DefaultThreadPool>);
}