#ifndef BOOST_BOUNDED_BUFFER_RING_BASED_H
#define BOOST_BOUNDED_BUFFER_RING_BASED_H

#include <condition_variable>
#include <mutex>

#include "ConcurrentQueueConcept.h"
#include "boost/call_traits.hpp"
#include "boost/circular_buffer.hpp"

template <class T>
class BoostBoundedBufferRingBased {
public:
    using container_type = boost::circular_buffer<T>;
    using size_type = typename container_type::size_type;
    using value_type = typename container_type::value_type;
    using param_type = typename boost::call_traits<value_type>::param_type;

    explicit BoostBoundedBufferRingBased(size_type capacity = 2048)
        : m_unread(0), m_container(capacity) {}

    BoostBoundedBufferRingBased(const BoostBoundedBufferRingBased&) = delete;
    BoostBoundedBufferRingBased& operator=(const BoostBoundedBufferRingBased&) = delete;

    bool push(param_type item) {
        {
            std::unique_lock lock(m_mutex);
            m_not_full.wait(lock, [&] { return is_not_full(); });
            m_container.push_front(item);
            ++m_unread;
        }
        m_not_empty.notify_one();
        return true;
    }

    bool try_push(param_type item) {
        {
            const std::lock_guard lock(m_mutex);
            if (!is_not_full()) return false;
            m_container.push_front(item);
            ++m_unread;
        }
        m_not_empty.notify_one();
        return true;
    }

    bool pop(value_type& item) {
        {
            std::unique_lock lock(m_mutex);
            m_not_empty.wait(lock, [&] { return is_not_empty(); });
            item = m_container[--m_unread];
        }
        m_not_full.notify_one();
        return true;
    }

    bool try_pop(value_type& item) {
        {
            const std::lock_guard lock(m_mutex);
            if (!is_not_empty()) return false;
            item = m_container[--m_unread];
        }
        m_not_full.notify_one();
        return true;
    }

private:
    [[nodiscard]] bool is_not_empty() const { return m_unread > 0; }
    [[nodiscard]] bool is_not_full() const { return m_unread < m_container.capacity(); }

    size_type m_unread;
    container_type m_container;
    std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
};

static_assert(ConcurrentQueue<BoostBoundedBufferRingBased<int>>,
              "boost_bounded_ring_based does not satisfy the ConcurrentQueue concept");

#endif  // BOOST_BOUNDED_BUFFER_RING_BASED_H
