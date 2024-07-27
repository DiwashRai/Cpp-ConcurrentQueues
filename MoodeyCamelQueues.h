#ifndef MOODEYCAMELQUEUES_H
#define MOODEYCAMELQUEUES_H

#include "ConcurrentQueueConcept.h"
#include "QueueTypeTraits.h"

#include "moodycamel/blockingconcurrentqueue.h"
#include "moodycamel/concurrentqueue.h"

template<typename T>
class MoodyCamelBlockingQueue {
public:
    explicit MoodyCamelBlockingQueue(std::size_t capacity = 256) : queue_(capacity) {}

    using value_type = T;

    bool push(const T& item) {
        queue_.enqueue(item);
        return true;
    }

    bool try_push(const T& item) {
        return queue_.try_enqueue(item);
    }

    bool pop(T& item) {
        queue_.wait_dequeue(item);
        return true;
    }

    bool try_pop(T& item) {
        return queue_.try_dequeue(item);
    }

private:
    moodycamel::BlockingConcurrentQueue<T> queue_;
};

template<typename T>
struct is_bounded<MoodyCamelBlockingQueue<T>> : std::true_type {};

static_assert(ConcurrentQueue<MoodyCamelBlockingQueue<int>>,
              "MoodyCamelBlockingQueue does not satisfy the ConcurrentQueue concept");

template<typename T>
class MoodyCamelLockFreeQueue {
public:
    using value_type = T;

    explicit MoodyCamelLockFreeQueue(std::size_t capacity) : queue_(capacity) {}

    bool push(const T& item) {
        return queue_.enqueue(item);
    }

    bool try_push(const T& item) {
        return queue_.try_enqueue(item);
    }

    bool pop(T& item) {
        return queue_.try_dequeue(item);
    }

    bool try_pop(T& item) {
        return queue_.try_dequeue(item);
    }

private:
    moodycamel::ConcurrentQueue<T> queue_;
};

template<typename T>
struct is_bounded<MoodyCamelLockFreeQueue<T>> : std::true_type {};

#endif //MOODEYCAMELQUEUES_H
