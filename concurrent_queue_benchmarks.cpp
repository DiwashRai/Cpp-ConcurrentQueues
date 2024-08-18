
#include <gtest/internal/gtest-internal.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <locale>
#include <numeric>
#include <print>
#include <vector>

#include "AtomicQueueAdapters.h"
#include "Barrier.h"
#include "BoostLockFreeAdapters.h"
#include "MoodeyCamelQueueAdapters.h"
#include "MutexBoostRingBufferQueue.h"
#include "MutexDequeQueue.h"
#include "MutexListQueue.h"
#include "MutexRingBufferQueue.h"
#include "StdAtomicSPSCQueue.h"
#include "StdAtomicMPMCQueue.h"

constexpr unsigned kQUEUE_SIZE = 16'384;
constexpr uint64_t kNUM_ITEMS = 1'008'000;

using namespace std::chrono;
using nano_t = nanoseconds::rep;

template <typename Queue>
Queue createQueue() {
    if constexpr (is_bounded_v<Queue>)
        return Queue(kQUEUE_SIZE);
    else
        return Queue();
}

enum class BenchmarkType {
    Balanced,
    SingleProducer,
    SingleConsumer,
};

std::string format_number(const long num) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << num;
    return ss.str();
}

std::string format_number(const long double num) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << std::setprecision(2) << num;
    return ss.str();
}

template <typename Queue>
void producer(Queue& queue, unsigned num_items, Barrier& barrier, std::atomic<nano_t>& start) {
    barrier.wait();
    const unsigned stop_flag = num_items + 1;

    const auto now = high_resolution_clock::now();
    nano_t expected = 0;
    start.compare_exchange_strong(expected,
                                  duration_cast<nanoseconds>(now.time_since_epoch()).count(),
                                  std::memory_order::acq_rel, std::memory_order::relaxed);

    for (unsigned n = 1; n <= stop_flag; ++n) {
        queue.push(n);
    }
}

template <typename Queue>
void single_producer(Queue& queue, const unsigned num_items, Barrier& barrier,
                     std::atomic<nano_t>& start, const unsigned consumer_count) {
    barrier.wait();

    const auto now = high_resolution_clock::now();
    nano_t expected = 0;
    start.compare_exchange_strong(expected,
                                  duration_cast<nanoseconds>(now.time_since_epoch()).count(),
                                  std::memory_order::acq_rel, std::memory_order::relaxed);

    for (unsigned n = 1; n <= num_items; ++n) {
        queue.push(n);
    }

    const unsigned stop_flag = num_items + 1;
    for (unsigned i = 0; i < consumer_count; ++i) {
        queue.push(stop_flag);
    }
}

template <typename Queue>
void consumer(Queue& queue, unsigned stop_flag, Barrier& barrier,
              std::atomic<unsigned>& active_consumers, nano_t& end, uint64_t& sum) {
    barrier.wait();

    uint64_t local_sum = 0;
    for (;;) {
        unsigned item;
        queue.pop(item);
        if (item == stop_flag) break;
        local_sum += item;
    }

    const auto now = high_resolution_clock::now();
    if (1 == active_consumers.fetch_sub(1, std::memory_order::acq_rel))
        end = duration_cast<nanoseconds>(now.time_since_epoch()).count();

    sum = local_sum;
}

template <typename Queue>
void single_consumer(Queue& queue, unsigned stop_flag, Barrier& barrier, nano_t& end,
                     unsigned producer_count, uint64_t& sum) {
    barrier.wait();

    uint64_t local_sum = 0;
    for (;;) {
        unsigned item;
        queue.pop(item);
        if (item == stop_flag && 0 == --producer_count) break;
        local_sum += item;
    }

    const auto now = high_resolution_clock::now();
    end = duration_cast<nanoseconds>(now.time_since_epoch()).count();

    sum = local_sum;
}

template <typename Queue>
nano_t balanced_benchmark_iteration(const unsigned thread_count,
                                    const uint64_t items_per_producer) {
    Barrier barrier;
    std::vector<std::thread> threads(thread_count * 2);
    std::vector<uint64_t> sums(thread_count, 0);

    std::atomic<nano_t> start{0};
    nano_t end = 0;
    std::atomic<unsigned> active_consumers{thread_count};
    auto queue = createQueue<Queue>();
    for (unsigned i = 0; i < thread_count; ++i) {
        threads[i] = std::thread(producer<Queue>, std::ref(queue), items_per_producer,
                                 std::ref(barrier), std::ref(start));
    }

    for (unsigned i = 0; i < thread_count; ++i) {
        threads[thread_count + i] =
            std::thread(consumer<Queue>, std::ref(queue), items_per_producer + 1, std::ref(barrier),
                        std::ref(active_consumers), std::ref(end), std::ref(sums[i]));
    }

    barrier.release(thread_count * 2);
    for (auto& t : threads) t.join();

    // check total sum
    const uint64_t expected_sum = thread_count * (items_per_producer * (1 + items_per_producer) / 2);
    uint64_t  total_sum = 0;
    for (const auto sum : sums)
        total_sum += sum;
    if (total_sum != expected_sum) {
        std::cerr << "ERROR: total sum is " << total_sum << " expected " << expected_sum * thread_count
                  << '\n';
    }

    return end - start.load(std::memory_order::relaxed);
}

template <typename Queue>
nano_t single_producer_benchmark_iteration(const unsigned consumer_count,
                                           const unsigned items_per_producer) {
    Barrier barrier;
    std::vector<std::thread> threads(consumer_count + 1);
    std::vector<uint64_t> sums(consumer_count);

    std::atomic<nano_t> start{0};
    nano_t end = 0;
    std::atomic<unsigned> active_consumers{consumer_count};
    auto queue = createQueue<Queue>();

    threads[0] = std::thread(single_producer<Queue>, std::ref(queue), items_per_producer,
                             std::ref(barrier), std::ref(start), consumer_count);

    for (unsigned i = 0; i < consumer_count; ++i) {
        threads[1 + i] =
            std::thread(consumer<Queue>, std::ref(queue), items_per_producer + 1, std::ref(barrier),
                        std::ref(active_consumers), std::ref(end), std::ref(sums[i]));
    }

    barrier.release(consumer_count + 1);
    for (auto& t : threads) t.join();

    return end - start;
}

template <typename Queue>
nano_t single_consumer_benchmark_iteration(const unsigned producer_count,
                                           const unsigned items_per_producer) {
    Barrier barrier;
    std::vector<std::thread> threads(producer_count + 1);
    uint64_t total_sum = 0;

    std::atomic<nano_t> start{0};
    nano_t end = 0;
    auto queue = createQueue<Queue>();

    for (unsigned i = 0; i < producer_count; ++i) {
        threads[i] = std::thread(producer<Queue>, std::ref(queue), items_per_producer,
                                 std::ref(barrier), std::ref(start));
    }

    threads[producer_count] =
        std::thread(single_consumer<Queue>, std::ref(queue), items_per_producer + 1,
                    std::ref(barrier), std::ref(end), producer_count, std::ref(total_sum));

    barrier.release(producer_count + 1);
    for (auto& t : threads) t.join();
    return end - start.load(std::memory_order::relaxed);
}

template <typename Queue>
void run_benchmark_set(char const* benchmark_name, BenchmarkType bt, unsigned min_threads,
                       unsigned max_threads) {
    constexpr unsigned RUNS = 3;
    std::cout << benchmark_name << '\n';

    for (unsigned thread_count = min_threads; thread_count <= max_threads; ++thread_count) {
        nano_t min_duration = std::numeric_limits<nano_t>::max();

        for (unsigned i = 0; i < RUNS; ++i) {
            switch (bt) {
                case BenchmarkType::Balanced: {
                    const uint64_t items_per_producer = kNUM_ITEMS / thread_count;
                    auto duration =
                        balanced_benchmark_iteration<Queue>(thread_count, items_per_producer);
                    min_duration = std::min(min_duration, duration);
                    break;
                }
                case BenchmarkType::SingleProducer: {
                    auto duration =
                        single_producer_benchmark_iteration<Queue>(thread_count, kNUM_ITEMS);
                    min_duration = std::min(min_duration, duration);
                    break;
                }
                case BenchmarkType::SingleConsumer: {
                    const uint64_t items_per_producer = kNUM_ITEMS / thread_count;
                    auto duration = single_consumer_benchmark_iteration<Queue>(thread_count,
                                                                               items_per_producer);
                    min_duration = std::min(min_duration, duration);
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
        }  // 1 - RUNS loop

        std::println("-> {:>2} Producer {:>2} Consumer - min_time: {:>12} ns - {:>15} msg/s",
                     bt == BenchmarkType::SingleProducer ? 1 : thread_count,
                     bt == BenchmarkType::SingleConsumer ? 1 : thread_count,
                     format_number(min_duration),
                     format_number(kNUM_ITEMS / (min_duration / static_cast<long double>(1e9))));
    }  // min_threads - max_threads loop
}

template <typename Queue>
void mpmc_benchmark(char const* benchmark_name, const unsigned min_threads,
                    const unsigned max_threads) {
    run_benchmark_set<Queue>(benchmark_name, BenchmarkType::Balanced, min_threads, max_threads);
}

template <typename Queue>
void spsc_benchmark(char const* benchmark_name) {
    run_benchmark_set<Queue>(benchmark_name, BenchmarkType::Balanced, 1, 1);
}

template <typename Queue>
void spmc_benchmark(char const* benchmark_name, unsigned int max_consumer_count) {
    run_benchmark_set<Queue>(benchmark_name, BenchmarkType::SingleProducer, 2, max_consumer_count);
}

template <typename Queue>
void mpsc_benchmark(char const* benchmark_name, unsigned int max_producer_count) {
    run_benchmark_set<Queue>(benchmark_name, BenchmarkType::SingleConsumer, 2, max_producer_count);
}

void spsc_benchmark_suite() {
    std::println("----------- SPSC Benchmarks -----------");

    spsc_benchmark<MutexDequeQueue<unsigned>>("MutexDequeQueue");
    //spsc_benchmark<MutexListQueue<unsigned>>("MutexListQueue");

    //spsc_benchmark<MutexRingBufferQueue<unsigned>>("MutexRingBufferQueue");
    //spsc_benchmark<MutexBoostRingBufferQueue<unsigned>>("MutexBoostRingBufferQueue");
    spsc_benchmark<StdAtomicSPSCQueue<unsigned, 16384>>("StdAtomicSPSCQueue");
    spsc_benchmark<StdAtomicMPMCQueue<unsigned, 16384>>("StdAtomicMPMCQueue");

    spsc_benchmark<BoostLockFreeSPSCQueue<unsigned, 16384>>("BoostLockFreeSPSCQueue");

    //spsc_benchmark<MoodyCamelBlockingQueue<unsigned>>("MoodyCamelBlockingQueue");
    //spsc_benchmark<MoodyCamelLockFreeQueue<unsigned>>("MoodyCamelLockFreeQueue");

    spsc_benchmark<AtomicQueueSPSCAdapter<unsigned, 16384>>("AtomicQueue(SPSC=true)");
    spsc_benchmark<OptimistAtomicQueueSPSCAdapter<unsigned, 16384>>("OptimistAtomicQueue(SPSC=true)");

    std::println();
}

void mpmc_benchmark_suite() {
    std::println("----------- MPMC Benchmarks -----------");

    mpmc_benchmark<MutexDequeQueue<unsigned>>("MutexDequeQueue", 2, 6);
    //mpmc_benchmark<MutexListQueue<unsigned>>("MutexListQueue", 2, 6);

    //mpmc_benchmark<MutexRingBufferQueue<unsigned>>("MutexRingBufferQueue", 2, 6);
    //mpmc_benchmark<MutexBoostRingBufferQueue<unsigned>>("MutexBoostRingBufferQueue", 2, 6);
    //mpmc_benchmark<StdAtomicSPSCQueue<unsigned, 16384>>("StdAtomicSPSCQueue", 2, 6);
    mpmc_benchmark<StdAtomicMPMCQueue<unsigned, 16384>>("StdAtomicMPMCQueue", 2, 6);

    mpmc_benchmark<BoostLockFreeQueue<unsigned, 16384>>("BoostLockFreeQueue", 2, 6);

    //mpmc_benchmark<MoodyCamelBlockingQueue<unsigned>>("MoodyCamelBlockingQueue", 2, 6);
    //mpmc_benchmark<MoodyCamelLockFreeQueue<unsigned>>("MoodyCamelLockFreeQueue", 2, 6);

    mpmc_benchmark<AtomicQueueAdapter<unsigned, 16384>>("AtomicQueue", 2, 6);
    //mpmc_benchmark<OptimistAtomicQueueAdapter<unsigned, 16384>>("OptimistAtomicQueue", 2, 6);

    std::println();
}

void spmc_benchmark_suite() {
    std::println("----------- SPMC Benchmarks -----------");

    spmc_benchmark<MutexDequeQueue<unsigned>>("MutexDequeQueue", 4);
    spmc_benchmark<MutexListQueue<unsigned>>("MutexListQueue", 4);

    spmc_benchmark<BoostLockFreeQueue<unsigned, 16384>>("BoostLockFreeQueue", 4);

    spmc_benchmark<MoodyCamelBlockingQueue<unsigned>>("MoodyCamelBlockingQueue", 4);
    spmc_benchmark<MoodyCamelLockFreeQueue<unsigned>>("MoodyCamelLockFreeQueue", 4);

    spmc_benchmark<AtomicQueueAdapter<unsigned, 16384>>("AtomicQueue", 4);

    std::println();
}

void mpsc_benchmark_suite() {
    std::println("----------- MPSC Benchmarks -----------");

    mpsc_benchmark<MutexDequeQueue<unsigned>>("MutexDequeQueue", 4);
    mpsc_benchmark<MutexListQueue<unsigned>>("MutexListQueue", 4);

    mpsc_benchmark<BoostLockFreeQueue<unsigned, 16384>>("BoostLockFreeQueue", 4);

    mpsc_benchmark<MoodyCamelBlockingQueue<unsigned>>("MoodyCamelBlockingQueue", 4);
    mpsc_benchmark<MoodyCamelLockFreeQueue<unsigned>>("MoodyCamelLockFreeQueue", 4);

    mpsc_benchmark<AtomicQueueAdapter<unsigned, 16384>>("AtomicQueue", 4);

    std::println();
}

int main(int argc, char* argv[]) {
    spsc_benchmark_suite();
    mpmc_benchmark_suite();
    //spmc_benchmark_suite();
    //mpsc_benchmark_suite();
}