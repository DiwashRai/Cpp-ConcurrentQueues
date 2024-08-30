
#include <emmintrin.h>
#include "cppcon2023/Fifo1.hpp"
#include "cppcon2023/Fifo2.hpp"
#include "cppcon2023/Fifo3.hpp"
#include "cppcon2023/Fifo4.hpp"

template <typename Queue, std::size_t SIZE>
struct FifoAdapter : Queue {
    using T = Queue::value_type;

    explicit FifoAdapter() : Queue(SIZE){};


    void push(const T& element) {
        while (!this->try_push(element))
            _mm_pause();
    }

    void pop(T& element) {
        while (!this->try_pop(element))
            _mm_pause();
    }
};

template<typename T, std::size_t SIZE>
using fifo1_adapter = FifoAdapter<Fifo1<T>, SIZE>;

template<typename T, std::size_t SIZE>
using fifo2_adapter = FifoAdapter<Fifo2<T>, SIZE>;

template<typename T, std::size_t SIZE>
using fifo3_adapter = FifoAdapter<Fifo3<T>, SIZE>;

template<typename T, std::size_t SIZE>
using fifo4_adapter = FifoAdapter<Fifo4<T>, SIZE>;
