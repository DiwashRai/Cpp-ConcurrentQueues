#ifndef TWOLOCKUNBOUNDEDQUEUE_H
#define TWOLOCKUNBOUNDEDQUEUE_H

#include <mutex>
#include <condition_variable>

template<typename T>
class TwoLockUnboundedQueue {
public:
    bool push(const T& item) {
        std::lock_guard lock(tail_mutex_);
        buffer_.push(item);
        not_empty_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock lock(head_mutex_);
        not_empty_.wait(lock, [&] { return !buffer_.empty(); });
        if (buffer_.empty())
            throw std::runtime_error("Buffer is empty");
        item = buffer_.front();
        buffer_.pop();
        return true;
    }

    bool try_pop(T& item) {
        {
            std::lock_guard lock(head_mutex_);
            if (buffer_.empty()) return false;
            item = buffer_.front();
            buffer_.pop();
        }
        return true;
    }

private:
    std::queue<T> buffer_;
    std::mutex tail_mutex_;
    std::mutex head_mutex_;
    std::condition_variable not_empty_;
};

#endif //TWOLOCKUNBOUNDEDQUEUE_H
