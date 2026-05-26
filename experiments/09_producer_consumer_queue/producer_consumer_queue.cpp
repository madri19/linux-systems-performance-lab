#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Number of items each producer generates.
constexpr int ITEMS_PER_PRODUCER = 500000;

// Benchmark result.
struct Result {
    long elapsed_us;
    double items_per_sec;
};

// ------------------------------------------------------------
// BlockingQueue
//
// A simple mutex-protected producer/consumer queue.
//
// All producers and consumers share:
// - one std::queue
// - one mutex
// - one condition variable
//
// This intentionally creates a shared synchronization point.
// ------------------------------------------------------------
class BlockingQueue {
public:
    void push(uint64_t value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(value);
        }

        cv.notify_one();
    }

    bool pop(uint64_t& value) {
        std::unique_lock<std::mutex> lock(mutex);

        cv.wait(lock, [this]() {
            return done || !queue.empty();
        });

        if (queue.empty()) {
            return false;
        }

        value = queue.front();
        queue.pop();

        return true;
    }

    void set_done() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }

        cv.notify_all();
    }

private:
    std::queue<uint64_t> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
};

// ------------------------------------------------------------
// Produces one synthetic item value.
// ------------------------------------------------------------
uint64_t produce_value(uint64_t seed) {
    uint64_t value = seed + 0x12345678;

    value ^= value << 7;
    value *= 1664525;
    value ^= value >> 13;

    return value;
}

// ------------------------------------------------------------
// Runs one producer/consumer queue benchmark.
// ------------------------------------------------------------
Result run_benchmark(int producer_count, int consumer_count) {
    BlockingQueue queue;

    std::atomic<int> producers_done{0};
    std::atomic<uint64_t> consumed_count{0};
    std::atomic<uint64_t> checksum{0};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    auto start = Clock::now();

    // --------------------------------------------------------
    // Start consumers first.
    //
    // Consumers block waiting for queue items.
    // --------------------------------------------------------
    for (int c = 0; c < consumer_count; ++c) {
        consumers.emplace_back([&]() {
            uint64_t value = 0;

            while (queue.pop(value)) {
                consumed_count.fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                checksum.fetch_xor(
                    value,
                    std::memory_order_relaxed
                );
            }
        });
    }

    // --------------------------------------------------------
    // Start producers.
    //
    // Each producer pushes ITEMS_PER_PRODUCER values.
    // --------------------------------------------------------
    for (int p = 0; p < producer_count; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                uint64_t value =
                    produce_value(
                        static_cast<uint64_t>(p)
                            * ITEMS_PER_PRODUCER
                            + i
                    );

                queue.push(value);
            }

            int finished =
                producers_done.fetch_add(
                    1,
                    std::memory_order_relaxed
                ) + 1;

            if (finished == producer_count) {
                queue.set_done();
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    for (auto& consumer : consumers) {
        consumer.join();
    }

    auto end = Clock::now();

    volatile uint64_t sink = checksum.load();
    (void)sink;

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    uint64_t total_items =
        static_cast<uint64_t>(producer_count)
            * ITEMS_PER_PRODUCER;

    std::cout << "Producers: " << producer_count << "\n";
    std::cout << "Consumers: " << consumer_count << "\n";
    std::cout << "Produced items: " << total_items << "\n";
    std::cout << "Consumed items: " << consumed_count.load() << "\n";
    std::cout << "Elapsed: " << elapsed_us << " us\n";

    return {
        elapsed_us,
        total_items / (elapsed_us / 1'000'000.0)
    };
}

int main() {
    std::vector<std::pair<int, int>> configurations = {
        {1, 1},
        {2, 2},
        {4, 4},
        {6, 6}
    };

    std::cout << "Producer/Consumer Queue Scaling Benchmark\n";
    std::cout
        << "Items per producer: "
        << ITEMS_PER_PRODUCER
        << "\n\n";

    double baseline = 0.0;

    for (auto [producers, consumers] : configurations) {
        auto result =
            run_benchmark(producers, consumers);

        std::cout
            << "Throughput: "
            << result.items_per_sec
            << " items/sec\n";

        if (producers == 1 && consumers == 1) {
            baseline = result.items_per_sec;
        }

        std::cout
            << "Throughput vs 1P/1C: "
            << result.items_per_sec / baseline
            << "x\n";

        std::cout << "-----------------------------\n";
    }

    return 0;
}
