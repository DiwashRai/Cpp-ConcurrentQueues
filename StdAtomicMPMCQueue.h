#ifndef STDATOMICMPMCQUEUE_H
#define STDATOMICMPMCQUEUE_H

#include <atomic>

template <typename T, std::size_t SIZE>
class StdAtomicMPMCQueue {
public:
    StdAtomicMPMCQueue() {
        for (size_t i = 0; i < SIZE; ++i)
            buffer_[i].seq_.store(i, std::memory_order_relaxed);
    }

    bool push(const T& item) {
        node_t* pNode = nullptr;
        auto write_idx = write_idx_.load(std::memory_order::relaxed);

        for (;;) {
            pNode = &buffer_[write_idx % SIZE];
            auto seq = pNode->seq_.load(std::memory_order::acquire);
            if (seq == write_idx) {
                if (write_idx_.compare_exchange_weak(write_idx, write_idx + 1,
                                                     std::memory_order::relaxed))
                    break;
            } else {
                write_idx = write_idx_.load(std::memory_order::relaxed);
            }
        }
        pNode->data_ = item;
        pNode->seq_.store(write_idx + 1, std::memory_order::release);
        return true;
    }

    bool pop(T& item) {
        node_t* pNode = nullptr;
        auto read_idx = read_idx_.load(std::memory_order::relaxed);

        for (;;) {
            pNode = &buffer_[read_idx % SIZE];
            auto seq = pNode->seq_.load(std::memory_order::acquire);
            if (seq == read_idx + 1) {
                if (read_idx_.compare_exchange_weak(read_idx, read_idx + 1,
                                                    std::memory_order::relaxed))
                    break;
            } else {
                read_idx = read_idx_.load(std::memory_order::relaxed);
            }
        }

        item = pNode->data_;
        pNode->seq_.store(read_idx + SIZE, std::memory_order::release);
        return true;
    }

private:
    struct alignas(64) node_t {
        std::atomic<std::size_t> seq_{0};
        T data_;
    };

    [[nodiscard]] static bool full(const std::size_t write_idx, const std::size_t read_idx) {
        return write_idx - read_idx >= SIZE;
    }

    [[nodiscard]] static bool empty(const std::size_t write_idx, const std::size_t read_idx) {
        return write_idx <= read_idx;
    }

    node_t buffer_[SIZE];
    std::atomic<std::size_t> write_idx_{0};
    std::atomic<std::size_t> read_idx_{0};
};

template <typename T>
class mpmc_bounded_queue {
public:
    mpmc_bounded_queue(size_t buffer_size)

        : buffer_(new cell_t[buffer_size]), buffer_mask_(buffer_size - 1) {
        assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));

        for (size_t i = 0; i != buffer_size; i += 1)
            buffer_[i].sequence_.store(i, std::memory_order_relaxed);
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    ~mpmc_bounded_queue() { delete[] buffer_; }

    bool enqueue(T const& data) {
        cell_t* cell;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);

        for (;;) {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)pos;
            if (dif == 0) {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0)
                return false;
            else
                pos = enqueue_pos_.load(std::memory_order_relaxed);
        }
        cell->data_ = data;
        cell->sequence_.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool dequeue(T& data) {
        cell_t* cell;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);

        for (;;) {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

            if (dif == 0) {
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0)
                return false;
            else
                pos = dequeue_pos_.load(std::memory_order_relaxed);
        }

        data = cell->data_;
        cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
        return true;
    }

private:
    struct cell_t {
        std::atomic<size_t> sequence_;
        T data_;
    };

    static size_t const cacheline_size = 64;
    typedef char cacheline_pad_t[cacheline_size];
    cacheline_pad_t pad0_;
    cell_t* const buffer_;
    size_t const buffer_mask_;
    cacheline_pad_t pad1_;
    std::atomic<size_t> enqueue_pos_;
    cacheline_pad_t pad2_;
    std::atomic<size_t> dequeue_pos_;
    cacheline_pad_t pad3_;
    mpmc_bounded_queue(mpmc_bounded_queue const&);
    void operator=(mpmc_bounded_queue const&);
};

#endif  // STDATOMICMPMCQUEUE_H
