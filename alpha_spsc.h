#ifndef ALPHA_SPSC_H
#define ALPHA_SPSC_H

#include <cstddef>
#include <atomic>
#include <memory>
#include <emmintrin.h>

namespace alpha {

constexpr std::size_t CACHE_LINE_SIZE = 64;

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    //push
    void push(const T& item) {
        while (!try_push(item))
            _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::acquire);
            if (full(tail, cached_head_))
                return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    //pop
    void pop(T& item) {
        while (!try_pop(item))
            _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::acquire);
            if (empty(cached_tail_, head))
                return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    unsigned capacity() const noexcept {
        return SIZE;
    }

private:
    [[nodiscard]] static bool full(const size_type tail, const size_type head) {
        return tail - head >= SIZE;
    }

    [[nodiscard]] static bool empty(const size_type tail, const size_type head) {
        return tail <= head;
    }

    T data_[SIZE];
    alignas(CACHE_LINE_SIZE) std::atomic<size_type> head_{0};
    alignas(CACHE_LINE_SIZE) size_type cached_head_ = 0;
    alignas(CACHE_LINE_SIZE) std::atomic<size_type> tail_{0};
    alignas(CACHE_LINE_SIZE) size_type cached_tail_ = 0;
    char padding_[CACHE_LINE_SIZE - sizeof(size_type) * 2];
};
}

namespace beta {

constexpr std::size_t CACHE_LINE_SIZE = 64;

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    //push
    void push(const T& item) {
        while (!try_push(item))
            _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::acquire);
            if (full(tail, cached_head_))
                return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    //pop
    void pop(T& item) {
        while (!try_pop(item))
            _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::acquire);
            if (empty(cached_tail_, head))
                return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    unsigned capacity() const noexcept {
        return SIZE;
    }

private:
    [[nodiscard]] static bool full(const size_type tail, const size_type head) {
        return tail - head >= SIZE;
    }

    [[nodiscard]] static bool empty(const size_type tail, const size_type head) {
        return tail <= head;
    }

    T data_[SIZE];
    std::atomic<size_type> head_{0};
    size_type cached_head_ = 0;
    std::atomic<size_type> tail_{0};
    size_type cached_tail_ = 0;
};
}

#endif //ALPHA_SPSC_H
