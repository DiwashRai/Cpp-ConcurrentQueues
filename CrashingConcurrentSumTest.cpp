
#include <gtest/gtest.h>

#include <queue>
#include <random>
#include <thread>

#include "random_num.h"

class ConcurrentSumTest : public testing::Test {};

// This test crashes because of race conditions in the queue.
TEST_F(ConcurrentSumTest, DISABLED_UnsafeQueuePushTest) {
    std::queue<int> queue;
    constexpr int num_threads = 10;

    std::vector<int> items = RandomNum::randomIntVec(0, 10, 1'000'000);
    const int total_sum = std::accumulate(items.begin(), items.end(), 0);

    std::vector<std::thread> push_threads;
    push_threads.reserve(10);
    for (int i = 0; i < num_threads; ++i) {
        push_threads.emplace_back([&items, &queue, i] {
            for (std::size_t j = i; j < items.size(); j += num_threads) {
                queue.emplace(items[j]);
            }
        });
    }
    for (auto& t : push_threads) t.join();

    int pushed_sum = 0;
    while (!queue.empty()) {
        pushed_sum += queue.front();
        queue.pop();
    }
    EXPECT_EQ(total_sum, pushed_sum);
}
