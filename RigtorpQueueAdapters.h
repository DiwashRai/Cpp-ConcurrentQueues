#ifndef RIGTORP_QUEUE_ADAPTERS_H
#define RIGTORP_QUEUE_ADAPTERS_H

#include "QueueTypeTraits.h"
#include "rigtorp/SPSCQueue.h"
#include <emmintrin.h>

template <typename T>
class RigtorpSPSCAdapter {
public:
    using value_type = T;

    RigtorpSPSCAdapter(const size_t capacity) : queue_(capacity) {}

    void push(const T& item) {
        queue_.push(item);
    }

    void pop(T& item) {
        while (!queue_.front())
            _mm_pause();
        item = *queue_.front();
        queue_.pop();
    }

private:
    rigtorp::SPSCQueue<T> queue_;
};

template<typename T>
struct is_bounded<RigtorpSPSCAdapter<T>> : std::true_type {};

#endif //RIGTORP_QUEUE_ADAPTERS_H
