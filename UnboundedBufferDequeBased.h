#ifndef BASICTSQUEUE_H
#define BASICTSQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>

#include "ConcurrentQueueConcept.h"

template<typename T>
class UnboundedBufferDequeBased {
public:
    using value_type = T;

    bool push(const T& item) {
        {
            const std::lock_guard lock(mutex_);
            buffer_.emplace_back(item);
        }
        not_empty_.notify_one();
        return true;
    }

    bool try_push(const T& item) {
        {
            const std::lock_guard lock(mutex_);
            buffer_.emplace_back(item);
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
        return true;
    }

    bool try_pop(T& item) {
        {
            const std::lock_guard lock(mutex_);
            if (buffer_.empty()) return false;
            item = buffer_.front();
            buffer_.pop_front();
        }
        return true;
    }

private:
    std::deque<T> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
};

static_assert(ConcurrentQueue<UnboundedBufferDequeBased<int>>,
              "UnboundedBufferDequeBased does not satisfy the ConcurrentQueue concept");

#endif //BASICTSQUEUE_H

