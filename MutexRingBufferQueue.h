#ifndef BLOCKINGBOUNDEDQUEUE_H
#define BLOCKINGBOUNDEDQUEUE_H

#include <mutex>
#include <condition_variable>

#include "ConcurrentQueueConcept.h"
#include "QueueTypeTraits.h"
#include "RingBuffer.h"

template<typename T>
class MutexRingBufferQueue {
public:
    using value_type = T;

    explicit MutexRingBufferQueue(std::size_t capacity = 256) : buffer_(capacity) {}

    bool push(const T& item) {
        {
            std::unique_lock lock(mutex_);
            not_full_.wait(lock, [&]{ return !buffer_.full(); });
            buffer_.push_back(item);
            max_size_ = std::max(max_size_, buffer_.size());
        }
        not_empty_.notify_one();
        return true;
    }

    bool try_push(const T& item) {
        {
            const std::lock_guard lock(mutex_);
            if (buffer_.full()) return false;
            buffer_.push_back(item);
            max_size_ = std::max(max_size_, buffer_.size());
        }
        not_empty_.notify_one();
        return true;
    }

    bool pop(T& item) {
        {
            std::unique_lock lock(mutex_);
            not_empty_.wait(lock, [&] { return !buffer_.empty(); });
            item = buffer_.front();
            buffer_.pop_front();
        }
        not_full_.notify_one();
        return true;
    }

    bool try_pop(T& item) {
        {
            const std::lock_guard lock(mutex_);
            if (buffer_.empty()) return false;
            item = buffer_.front();
            buffer_.pop_front();
        }
        not_full_.notify_one();
        return true;
    }

    std::size_t max_size() const { return max_size_; }

private:
    RingBuffer<T> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::size_t max_size_ = 0;
};

static_assert(ConcurrentQueue<MutexRingBufferQueue<int>>,
              "BoundedBufferRingBased does not satisfy the ConcurrentQueue concept");

template<typename T>
struct is_bounded<MutexRingBufferQueue<T>> : std::true_type {};

#endif //BLOCKINGBOUNDEDQUEUE_H
