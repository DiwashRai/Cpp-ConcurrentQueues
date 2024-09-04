#ifndef ALPHA_SPSC_H
#define ALPHA_SPSC_H

#include <emmintrin.h>

#include <atomic>
#include <cstddef>
#include <memory>

constexpr std::size_t CACHE_LINE_SIZE = 64;


namespace alpha {

//=================================================================================
//                 SPSC with all optimisations
//=================================================================================

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    // push
    void push(const T& item) {
        while (!try_push(item)) _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::acquire);
            if (full(tail, cached_head_)) return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    // pop
    void pop(T& item) {
        while (!try_pop(item)) _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::acquire);
            if (empty(cached_tail_, head)) return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    [[nodiscard]] unsigned capacity() const noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order::relaxed) == tail_.load(std::memory_order::relaxed);
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
    char padding_[CACHE_LINE_SIZE - sizeof(size_type)];
};
}  // namespace alpha

namespace bravo {

//=================================================================================
//                 No alignas / with false sharing
//=================================================================================

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    // push
    void push(const T& item) {
        while (!try_push(item)) _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::acquire);
            if (full(tail, cached_head_)) return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    // pop
    void pop(T& item) {
        while (!try_pop(item)) _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::acquire);
            if (empty(cached_tail_, head)) return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    [[nodiscard]] unsigned capacity() const noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order::relaxed) == tail_.load(std::memory_order::relaxed);
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
}  // namespace bravo

namespace charlie {

//=================================================================================
//                 Sequentially consistent memory order
//=================================================================================

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    // push
    void push(const T& item) {
        while (!try_push(item)) _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::seq_cst);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::seq_cst);
            if (full(tail, cached_head_)) return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::seq_cst);
        return true;
    }

    // pop
    void pop(T& item) {
        while (!try_pop(item)) _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::seq_cst);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::seq_cst);
            if (empty(cached_tail_, head)) return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::seq_cst);
        return true;
    }

    [[nodiscard]] unsigned capacity() const noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order::relaxed) == tail_.load(std::memory_order::seq_cst);
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
    char padding_[CACHE_LINE_SIZE - sizeof(size_type)];
};
}  // namespace charlie

namespace delta {

//=================================================================================
//                 No cached head/tail indexes
//=================================================================================

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() = default;
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    // push
    void push(const T& item) {
        while (!try_push(item)) _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        auto head = head_.load(std::memory_order::acquire);
        if (full(tail, head))
            return false;
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    // pop
    void pop(T& item) {
        while (!try_pop(item)) _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        auto tail = tail_.load(std::memory_order::acquire);
        if (empty(tail, head))
            return false;
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    [[nodiscard]] unsigned capacity() const noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order::relaxed) == tail_.load(std::memory_order::relaxed);
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
    alignas(CACHE_LINE_SIZE) std::atomic<size_type> tail_{0};
    char padding_[CACHE_LINE_SIZE - sizeof(size_type)];
};
}  // namespace delta

namespace echo {

//=================================================================================
//                 Buffer array on heap instead of stack
//=================================================================================

template <typename T, unsigned SIZE, T NIL>
class spsc {
public:
    using value_type = T;
    using size_type = std::size_t;

    spsc() : data_(new T[SIZE]) {}
    spsc(spsc&) = delete;
    spsc& operator=(spsc&) = delete;

    // push
    void push(const T& item) {
        while (!try_push(item)) _mm_pause();
    }

    bool try_push(const T& item) {
        auto tail = tail_.load(std::memory_order::relaxed);
        if (full(tail, cached_head_)) {
            cached_head_ = head_.load(std::memory_order::acquire);
            if (full(tail, cached_head_)) return false;
        }
        new (&data_[tail % SIZE]) T(item);
        tail_.store(tail + 1, std::memory_order::release);
        return true;
    }

    // pop
    void pop(T& item) {
        while (!try_pop(item)) _mm_pause();
    }

    bool try_pop(T& item) {
        auto head = head_.load(std::memory_order::relaxed);
        if (empty(cached_tail_, head)) {
            cached_tail_ = tail_.load(std::memory_order::acquire);
            if (empty(cached_tail_, head)) return false;
        }
        item = data_[head % SIZE];
        head_.store(head + 1, std::memory_order::release);
        return true;
    }

    [[nodiscard]] unsigned capacity() const noexcept { return SIZE; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order::relaxed) == tail_.load(std::memory_order::relaxed);
    }

private:
    [[nodiscard]] static bool full(const size_type tail, const size_type head) {
        return tail - head >= SIZE;
    }

    [[nodiscard]] static bool empty(const size_type tail, const size_type head) {
        return tail <= head;
    }

    std::unique_ptr<T[]> data_;
    alignas(CACHE_LINE_SIZE) std::atomic<size_type> head_{0};
    alignas(CACHE_LINE_SIZE) size_type cached_head_ = 0;
    alignas(CACHE_LINE_SIZE) std::atomic<size_type> tail_{0};
    alignas(CACHE_LINE_SIZE) size_type cached_tail_ = 0;
    char padding_[CACHE_LINE_SIZE - sizeof(size_type)];
};
}  // namespace echo

#endif  // ALPHA_SPSC_H
