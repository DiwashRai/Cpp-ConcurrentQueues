#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <vector>

template<typename T>
class RingBuffer {
public:

    explicit RingBuffer(std::size_t capacity) : buffer_(capacity) {}

    [[nodiscard]] T& front() { return buffer_[head_]; }

    [[nodiscard]] T& back() { return buffer_[tail_ == 0 ? capacity() - 1 : tail_ - 1]; }

    void push_back(const T& item) {
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity();
        if (size_ == capacity()) {
            head_ = (head_ + 1) % capacity();
        } else {
            ++size_;
        }
    }

    void pop_front() {
        head_ = (head_ + 1) % capacity();
        --size_;
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return size_ == 0; }
    [[nodiscard]] bool full() const { return size_ == capacity(); }

private:
    [[nodiscard]] std::size_t capacity() const { return buffer_.size(); }

    std::vector<T> buffer_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
};

#endif  // RINGBUFFER_H
