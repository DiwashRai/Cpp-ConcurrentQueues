#ifndef ATOMICQUEUEADAPTERS_H
#define ATOMICQUEUEADAPTERS_H

#include "atomic_queue/atomic_queue.h"

template <typename T, unsigned SIZE>
class AtomicQueueSPSCAdapter {
public:
    using value_type = T;

    bool push(const T& item) {
        queue_.push(item);
        return true;
    }

    bool pop(T& item) {
        item = queue_.pop();
        return true;
    }

private:
    atomic_queue::RetryDecorator<atomic_queue::AtomicQueue<T, SIZE, T{}, false, false, false, true>>
        queue_;
};

template <typename T, unsigned SIZE>
class AtomicQueueAdapter {
public:
    using value_type = T;

    bool push(const T& item) {
        queue_.push(item);
        return true;
    }

    bool pop(T& item) {
        item = queue_.pop();
        return true;
    }

private:
    atomic_queue::RetryDecorator<atomic_queue::AtomicQueue<T, SIZE>> queue_;
};

template <typename T, unsigned SIZE>
class OptimistAtomicQueueSPSCAdapter {
public:
    using value_type = T;

    bool push(const T& item) {
        queue_.push(item);
        return true;
    }

    bool pop(T& item) {
        item = queue_.pop();
        return true;
    }

private:
    atomic_queue::AtomicQueue<T, SIZE, T{}, false, false, false, true> queue_;
};

template <typename T, unsigned SIZE>
class OptimistAtomicQueueAdapter {
public:
    using value_type = T;

    bool push(const T& item) {
        queue_.push(item);
        return true;
    }

    bool pop(T& item) {
        item = queue_.pop();
        return true;
    }

private:
    atomic_queue::AtomicQueue<T, SIZE> queue_;
};

#endif  // ATOMICQUEUEADAPTERS_H
