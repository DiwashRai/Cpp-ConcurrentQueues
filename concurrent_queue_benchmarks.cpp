
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>

#include "random_num.h"
#include "UnboundedBufferDequeBased.h"
#include "BoundedBufferRingBased.h"
#include "BoostBoundedBufferRingBased.h"
#include "MoodeyCamelQueues.h"

constexpr int kQUEUE_SIZE = 4096;

template<typename Queue>
Queue
createQueue() {
    if constexpr (is_bounded_v<Queue>)
        return Queue(kQUEUE_SIZE);
    else
        return Queue();
}

template <typename queue_t>
std::pair<std::chrono::microseconds, std::chrono::microseconds> run_benchmark(
    const std::vector<std::vector<int>>& item_sets, const int num_threads, const long total_sum) {

    std::atomic<int> ready_threads = 0;
    std::vector<std::chrono::microseconds> push_timings(num_threads);
    std::vector<std::chrono::microseconds> pop_timings(num_threads);
    auto queue = createQueue<queue_t>();

    auto producer = [&](const std::vector<int>& items, const std::size_t thread_idx) {
        ready_threads.fetch_add(1, std::memory_order_relaxed);
        while (ready_threads.load(std::memory_order_relaxed) != num_threads * 2) {
        }

        const auto start_time = std::chrono::high_resolution_clock::now();

        for (const auto& item : items) queue.push(item);
        queue.push(0);  // terminate signal

        const auto end_time = std::chrono::high_resolution_clock::now();
        push_timings[thread_idx] =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    };

    std::vector<int> sums;
    auto consumer = [&](const std::size_t thread_idx) {
        ready_threads.fetch_add(1, std::memory_order_relaxed);
        while (ready_threads.load(std::memory_order_relaxed) != num_threads * 2) {
        }

        const auto start_time = std::chrono::high_resolution_clock::now();

        int n = -1;
        int local_sum = 0;
        for (;;) {
            queue.pop(n);
            if (n == 0) break;
            local_sum += n;
        }

        const auto end_time = std::chrono::high_resolution_clock::now();
        pop_timings[thread_idx] =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        sums.emplace_back(local_sum);
    };

    std::vector<std::thread> push_threads;
    push_threads.reserve(num_threads);
    std::vector<std::thread> pop_threads;
    pop_threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i) {
        push_threads.emplace_back(producer, item_sets[i], i);
        pop_threads.emplace_back(consumer, i);
    }
    for (auto& t : push_threads) t.join();
    for (auto& t : pop_threads) t.join();

    std::chrono::microseconds total_push_time = std::chrono::microseconds::zero();
    for (const auto& time : push_timings) total_push_time += time;
    std::chrono::microseconds total_pop_time = std::chrono::microseconds::zero();
    for (const auto& time : pop_timings) total_pop_time += time;

    const long local_total_sum = std::accumulate(sums.begin(), sums.end(), 0L);
    if (local_total_sum != total_sum) {
            std::cerr << "Error: total sum mismatch\n";
    }

    return {total_push_time, total_pop_time};
}

int main(int argc, char* argv[]) {
    // - benchmark constants
    constexpr int kNUM_ITEMS = 1'000'000;
    constexpr int kTHREADS = 8;

    // - set up data
    std::vector<std::vector<int>> item_sets;
    item_sets.reserve(kTHREADS);
    for (std::size_t i = 0; i < kTHREADS; ++i)
        item_sets.emplace_back(RandomNum::randomIntVec(1, 10, kNUM_ITEMS));
    long total_sum = 0;
    for (const auto& items : item_sets)
            total_sum += std::accumulate(items.begin(), items.end(), 0);

    // - run benchmarks using macros
    /*
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto [push_time, pop_time] =
            run_benchmark<UnboundedBufferDequeBased<int>>(item_sets, kTHREADS, total_sum);
        std::cout << "UnboundedBufferDequeBased push time: " << push_time << '\n';
        std::cout << "UnboundedBufferDequeBased pop time: " << pop_time << '\n';
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::cout
            << "Total time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time) << "\n\n";
    }

    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto [push_time, pop_time] =
            run_benchmark<BoundedBufferRingBased<int>>(item_sets, kTHREADS, total_sum);
        std::cout << "BoundedBufferRingBased push time: " << push_time << '\n';
        std::cout << "BoundedBufferRingBased pop time: " << pop_time<< "\n\n";
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::cout
            << "Total time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time) << "\n\n";
    }

    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto [push_time, pop_time] =
            run_benchmark<BoostBoundedBufferRingBased<int>>(item_sets, kTHREADS, total_sum);
        std::cout << "BoostBoundedBufferRingBased push time: " << push_time << '\n';
        std::cout << "BoostBoundedBufferRingBased pop time: " << pop_time<< "\n\n";
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::cout
            << "Total time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time) << "\n\n";
    }
    */
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto [push_time, pop_time] =
            run_benchmark<MoodyCamelBlockingQueue<int>>(item_sets, kTHREADS, total_sum);
        std::cout << "MoodyCamelBlockingQueue push time: " << push_time << '\n';
        std::cout << "MoodyCamelBlockingQueue pop time: " << pop_time<< "\n\n";
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::cout
            << "Total time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time) << "\n\n";
    }
    /*
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto [push_time, pop_time] =
            run_benchmark<MoodyCamelLockFreeQueue<int>>(item_sets, kTHREADS, total_sum);
        std::cout << "MoodyCamelLockFreeQueue push time: " << push_time << '\n';
        std::cout << "MoodyCamelLockFreeQueue pop time: " << pop_time<< "\n\n";
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::cout
            << "Total time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time) << "\n\n";
    }
    */
}