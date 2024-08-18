
#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "MoodeyCamelQueueAdapters.h"
#include "MutexBoostRingBufferQueue.h"
#include "MutexDequeQueue.h"
#include "MutexRingBufferQueue.h"
#include "random_num.h"

using QueueTypes = testing::Types<
    MutexDequeQueue<int>,
    MutexRingBufferQueue<int>,
    MutexBoostRingBufferQueue<int>,
    MoodyCamelBlockingQueue<int>
>;

using BoundedQueueTypes = testing::Types<
    MutexRingBufferQueue<int>,
    MutexBoostRingBufferQueue<int>
>;

using UnboundedQueueTypes = testing::Types<
    MutexDequeQueue<int>
>;

template <typename T>
class ConcurrentQueueTypedTest : public testing::Test {};

TYPED_TEST_SUITE(ConcurrentQueueTypedTest, QueueTypes);

template <typename T>
class UnboundedQueueTypedTest : public testing::Test {};

TYPED_TEST_SUITE(UnboundedQueueTypedTest, UnboundedQueueTypes);

template <typename T>
class BoundedQueueTypedTest : public testing::Test {};

TYPED_TEST_SUITE(BoundedQueueTypedTest, BoundedQueueTypes);

/**************************************************************
                        All Queue Types
***************************************************************/

TYPED_TEST(ConcurrentQueueTypedTest, try_popTest) {
    TypeParam queue;

    int out = -1;
    EXPECT_FALSE(queue.try_pop(out));
    EXPECT_EQ(out, -1);

    queue.push(1);
    EXPECT_TRUE(queue.try_pop(out));
    EXPECT_EQ(out, 1);
}

TYPED_TEST(ConcurrentQueueTypedTest, popTest) {
    TypeParam queue;
    std::atomic<int> out_val = -1;

    std::thread pop_thread([&] {
        int out = -1;
        queue.pop(out);
        out_val = out;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_EQ(out_val, -1);

    queue.push(7);
    pop_thread.join();
    EXPECT_EQ(out_val, 7);
}

TYPED_TEST(ConcurrentQueueTypedTest, EnqueueDequeTest) {
    TypeParam queue;
    constexpr int total_pushes = 1000000;
    constexpr int num_threads = 4;
    std::atomic<int> push_sum = 0;
    std::atomic<int> pop_sum = 0;

    auto producer = [&] {
        for (int i = 0; i < total_pushes; ++i) {
            const int n = RandomNum::randomInt(1, 10);
            push_sum += n;
            queue.push(n);
        }
        queue.push(0);
    };
    auto consumer = [&] {
        while (true) {
            int n = -1;
            queue.pop(n);
            if (n == 0) return;
            pop_sum += n;
        }
    };

    std::vector<std::thread> push_threads;
    push_threads.reserve(num_threads);
    std::vector<std::thread> pop_threads;
    pop_threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        push_threads.emplace_back(producer);
        pop_threads.emplace_back(consumer);
    }
    for (auto& t : push_threads) t.join();
    for (auto& t : pop_threads) t.join();

    EXPECT_EQ(push_sum, pop_sum);
}

namespace {

template <typename TypeParam>
void sequentialPushPop(TypeParam& queue) {
    auto items = RandomNum::randomIntVec(1, 10000, 100);
    for (const int n : items) {
        EXPECT_TRUE(queue.try_push(n));
    }

    int n = -1;
    for (const int expected : items) {
        EXPECT_TRUE(queue.try_pop(n));
        EXPECT_EQ(n, expected);
    }
}

} // namespace

TYPED_TEST(UnboundedQueueTypedTest, UnboundedSequentialPushPopTest) {
    TypeParam queue;
    sequentialPushPop(queue);
}

TYPED_TEST(BoundedQueueTypedTest, boundedSequentialPushPopTest) {
    TypeParam queue(100);
    sequentialPushPop(queue);
}

/*****************************************************************
                       Unbounded Queue Types
******************************************************************/


TYPED_TEST(UnboundedQueueTypedTest, EnqueueTest) {
    TypeParam queue;
    constexpr int num_threads = 10;

    std::vector<int> items = RandomNum::randomIntVec(0, 10, 1'000'000);
    const int total_sum = std::accumulate(items.begin(), items.end(), 0);

    std::vector<std::thread> push_threads;
    push_threads.reserve(10);
    for (int i = 0; i < num_threads; ++i) {
        push_threads.emplace_back([&items, &queue, i] {
            for (std::size_t j = i; j < items.size(); j += num_threads) {
                queue.push(items[j]);
            }
        });
    }
    for (auto& t : push_threads) t.join();

    int pushed_sum = 0;
    int n = 0;
    while (queue.try_pop(n)) pushed_sum += n;
    EXPECT_EQ(total_sum, pushed_sum);
}

/******************************************************************
                        Bounded Queue Types
*******************************************************************/

TYPED_TEST(BoundedQueueTypedTest, try_pushTest) {
    TypeParam queue(4);
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    EXPECT_TRUE(queue.try_push(4));
    EXPECT_FALSE(queue.try_push(5));
}

TYPED_TEST(BoundedQueueTypedTest, pushTest) {
    TypeParam queue(2);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));

    std::atomic<bool> pushed = false;
    std::thread push_thread([&] {
        pushed = queue.push(3);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_FALSE(pushed);

    int n = -1;
    EXPECT_TRUE(queue.pop(n));
    EXPECT_EQ(n, 1);

    push_thread.join();
    EXPECT_TRUE(pushed);
}

