
#include <atomic>
#include <chrono>
#include <iostream>
#include <locale>
#include <print>
#include <vector>

#include "Barrier.h"
#include "BoostBoundedBufferRingBased.h"
#include "BoundedBufferRingBased.h"
#include "MoodeyCamelQueues.h"
#include "UnboundedBufferDequeBased.h"

constexpr unsigned kQUEUE_SIZE = 4096;
constexpr unsigned kNUM_ITEMS = 1'008'000;

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
              std::atomic<unsigned>& active_consumers, nano_t& end, unsigned& sum) {
    barrier.wait();

    unsigned local_sum = 0;
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
                     unsigned producer_count, unsigned& sum) {
    barrier.wait();

    unsigned local_sum = 0;
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
                                    const unsigned items_per_producer) {
    Barrier barrier;
    std::vector<std::thread> threads(thread_count * 2);
    std::vector<unsigned> sums(thread_count);

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

    return end - start.load(std::memory_order::relaxed);
}

template <typename Queue>
nano_t single_producer_benchmark_iteration(const unsigned consumer_count,
                                           const unsigned items_per_producer) {
    Barrier barrier;
    std::vector<std::thread> threads(consumer_count + 1);
    std::vector<unsigned> sums(consumer_count);

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
    unsigned total_sum = 0;

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
                    const unsigned items_per_producer = kNUM_ITEMS / thread_count;
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
                    const unsigned items_per_producer = kNUM_ITEMS / thread_count;
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

        std::println("-> {:<2} Producer {:<2} Consumer - min_time: {:<12} ns - {:<12} msg/s",
                     bt == BenchmarkType::SingleProducer ? 1 : thread_count,
                     bt == BenchmarkType::SingleConsumer ? 1 : thread_count,
                     format_number(min_duration),
                     format_number(kNUM_ITEMS / (min_duration / static_cast<long double>(1e9))));
    }  // min_threads - max_threads loop
}

template <typename Queue>
void mpmc_benchmark(char const* benchmark_name, unsigned int min_threads,
                    unsigned int max_threads) {
    run_benchmark_set<Queue>(benchmark_name, BenchmarkType::Balanced, 2, 4);
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

    spsc_benchmark<UnboundedBufferDequeBased<unsigned>>("UnboundedBufferDequeBased");

    std::println();
}

void mpmc_benchmark_suite() {
    std::println("----------- MPMC Benchmarks -----------");

    mpmc_benchmark<UnboundedBufferDequeBased<unsigned>>("UnboundedBufferDequeBased", 1, 4);

    std::println();
}

void spmc_benchmark_suite() {
    std::println("----------- SPMC Benchmarks -----------");

    spmc_benchmark<UnboundedBufferDequeBased<unsigned>>("UnboundedBufferDequeBased", 4);

    std::println();
}

void mpsc_benchmark_suite() {
    std::println("----------- MPSC Benchmarks -----------");

    mpsc_benchmark<UnboundedBufferDequeBased<unsigned>>("UnboundedBufferDequeBased", 4);

    std::println();
}

int main(int argc, char* argv[]) {
    spsc_benchmark_suite();
    mpmc_benchmark_suite();
    spmc_benchmark_suite();
    mpsc_benchmark_suite();
}