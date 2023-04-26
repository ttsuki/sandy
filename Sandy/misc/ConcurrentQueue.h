/// @file
///	@brief   sandy::mf::concurrent_queue
///	@author  (C) 2023 ttsuki

#pragma once

#include <cstddef>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <limits>
#include <chrono>

namespace sandy
{
    template <class T>
    class ConcurrentQueue final
    {
        mutable std::recursive_mutex mutex_{};
        const size_t capacity_{};

        std::condition_variable_any can_produce_{};
        std::condition_variable_any can_consume_{};
        std::queue<std::optional<T>> queue_{};
        bool closed_{};

    public:
        ConcurrentQueue(
            size_t limit = std::numeric_limits<size_t>::max())
            : capacity_(limit) { }

        ConcurrentQueue(const ConcurrentQueue& other) = delete;
        ConcurrentQueue(ConcurrentQueue&& other) noexcept = delete;
        ConcurrentQueue& operator=(const ConcurrentQueue& other) = delete;
        ConcurrentQueue& operator=(ConcurrentQueue&& other) noexcept = delete;
        ~ConcurrentQueue() = default;

        [[nodiscard]] bool closed() const noexcept
        {
            std::unique_lock lock(mutex_);
            return queue_.empty() && closed_;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            std::unique_lock lock(mutex_);
            return queue_.empty();
        }

        [[nodiscard]] size_t capacity() const noexcept
        {
            return capacity_;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            std::unique_lock lock(mutex_);
            return queue_.size();
        }

        /// Pushes value.
        template <class... U>
        bool emplace(U&&... val)
        {
            std::unique_lock lock(mutex_);
            if (closed_) { return false; }

            can_produce_.wait(lock, [&]
            {
                if (queue_.size() < capacity_)
                {
                    queue_.emplace(std::in_place, std::forward<U>(val)...);
                    can_consume_.notify_all();
                    return true;
                }
                return false;
            });

            return true;
        }

        /// Closes queue.
        void close()
        {
            std::unique_lock lock(mutex_);
            closed_ = true;
            can_consume_.notify_all();
        }

        /// Tries pop value, may returns nullopt if queue is empty or closed.
        [[nodiscard]] std::optional<T> try_pop()
        {
            std::unique_lock lock(mutex_);

            if (!queue_.empty())
            {
                std::optional<T> ret = std::move(queue_.front());
                queue_.pop();
                can_produce_.notify_all();
                return ret;
            }

            return std::nullopt;
        }


        /// Waits for value.
        /// may return nullopt if queue is closed.
        [[nodiscard]] std::optional<T> pop_wait()
        {
            std::unique_lock lock(mutex_);

            std::optional<T> ret{};
            can_consume_.wait(lock, [&]
            {
                return closed() || (ret = try_pop()).has_value();
            });

            return ret;
        }

        /// Waits for value.
        /// may returns nullopt if queue is closed, or empty till timed out.
        template <class Rep, class Period>
        [[nodiscard]] std::optional<T> pop_wait_for(std::chrono::duration<Rep, Period> timeout)
        {
            std::unique_lock lock(mutex_);

            std::optional<T> ret{};
            can_consume_.wait_for(lock, timeout, [&]
            {
                return closed() || (ret = try_pop()).has_value();
            });

            return ret;
        }
    };
}
