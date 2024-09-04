#ifndef STD_ATOMIC_SPSC_QUEUE_H
#define STD_ATOMIC_SPSC_QUEUE_H

#include <atomic>

template <typename T, std::size_t SIZE>
class StdAtomicSPSCQueue {
public:
    bool push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        for (;;) {
            if (full(tail, cached_head_)) {
                cached_head_ = head_.load(std::memory_order::acquire);
                if (full(tail, cached_head_)) {
                    continue;
                }
            }
            new (&buffer_[tail % SIZE].data_) T(item);
            tail_.store(tail + 1, std::memory_order::release);
            return true;
        }
    }

    bool pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        for (;;) {
            if (empty(cached_tail_, head)) {
                cached_tail_ = tail_.load(std::memory_order::acquire);
                if (empty(cached_tail_, head)) {
                    continue;
                }
            }
            item = buffer_[head % SIZE].data_;
            head_.store(head + 1, std::memory_order::release);
            return true;
        }
    }

private:
    struct alignas(64) node_t {
        T data_;
    };

    [[nodiscard]] static bool full(const std::size_t tail, const std::size_t head) {
        return tail - head >= SIZE;
    }

    [[nodiscard]] static bool empty(const std::size_t tail, const std::size_t head) {
        return tail <= head;
    }

    node_t buffer_[SIZE];
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::size_t cached_head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    alignas(64) std::size_t cached_tail_{0};
};

#endif  // STD_ATOMIC_SPSC_QUEUE_H
