#ifndef BARRIER_H
#define BARRIER_H

#include <atomic>

class Barrier {
public:
    void wait() {
        count_.fetch_add(1, std::memory_order::acquire);
        while (count_.load(std::memory_order::relaxed) != 0) {}
    }

    void release(const unsigned int expected_count) {
        while (count_.load(std::memory_order::relaxed) != expected_count) {}
        count_.store(0, std::memory_order::release);
    }

private:
    std::atomic<unsigned int> count_;
};

#endif //BARRIER_H
