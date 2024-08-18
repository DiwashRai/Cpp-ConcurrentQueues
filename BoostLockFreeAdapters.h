#ifndef BOOSTLOCKFREEADAPTERS_H
#define BOOSTLOCKFREEADAPTERS_H

#include "boost/lockfree/spsc_queue.hpp"
#include "boost/lockfree/queue.hpp"

template<typename T, std::size_t SIZE>
class BoostLockFreeSPSCQueue {
public:
     using value_type = T;

    bool push(const T& item) {
        while (!queue_.push(item)){}
        return true;
    }

    bool pop(T& item) {
        while (!queue_.pop(item)) {}
        return true;
    }

private:
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<SIZE>> queue_;
};

template<typename T, std::size_t SIZE>
class BoostLockFreeQueue {
public:
     using value_type = T;

    bool push(const T& item) {
        while (!queue_.bounded_push(item)){}
        return true;
    }

    bool pop(T& item) {
        while (!queue_.pop(item)) {}
        return true;
    }

private:
    boost::lockfree::queue<T, boost::lockfree::capacity<SIZE>> queue_;
};

#endif //BOOSTLOCKFREEADAPTERS_H
