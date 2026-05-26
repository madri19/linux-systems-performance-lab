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

// Number of items transferred through each queue.
constexpr uint64_t ITEM_COUNT = 5'000'000;

// Fixed ring capacity.
//
// Power-of-two-sized capacity would allow bitmask wrapping,
// but this version keeps modulo arithmetic explicit.
constexpr size_t RING_CAPACITY = 1024 * 64;

// Benchmark result.
struct Result {
    long elapsed_us;
    double items_per_sec;
};

// ------------------------------------------------------------
// Creates one synthetic item value.
// ------------------------------------------------------------
uint64_t make_value(uint64_t seed) {
    uint64_t value = seed + 0x12345678;

    value ^= value << 7;
    value *= 1664525;
    value ^= value >> 13;

    return value;
}

// ------------------------------------------------------------
// MutexQueue
//
// General-purpose blocking queue using:
// - std::queue
// - std::mutex
// - std::condition_variable
//
// This supports producer/consumer coordination but pays
// general synchronization overhead.
// ------------------------------------------------------------
class MutexQueue {
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
// Single-producer/single-consumer ring buffer.
//
// Ownership model:
// - only producer writes head
// - only consumer writes tail
//
// Atomics are still used so both sides can observe each other's
// progress safely across threads.
// ------------------------------------------------------------
class SPSCRingBuffer {
public:
    explicit SPSCRingBuffer(size_t capacity)
        : buffer(capacity),
          capacity(capacity) {}

    bool push(uint64_t value) {
        size_t current_head =
            head.load(std::memory_order_relaxed);

        size_t next_head =
            (current_head + 1) % capacity;

        // Acquire tail to observe consumer progress.
        if (next_head == tail.load(std::memory_order_acquire)) {
            return false;
        }

        buffer[current_head] = value;

        // Release publishes item to consumer.
        head.store(next_head, std::memory_order_release);

        return true;
    }

    bool pop(uint64_t& value) {
        size_t current_tail =
            tail.load(std::memory_order_relaxed);

        // Acquire head to observe producer progress.
        if (current_tail == head.load(std::memory_order_acquire)) {
            return false;
        }

        value = buffer[current_tail];

        // Release publishes consumed slot to producer.
        tail.store(
            (current_tail + 1) % capacity,
            std::memory_order_release
        );

        return true;
    }

private:
    std::vector<uint64_t> buffer;
    size_t capacity;

    // Separate head/tail onto different cache lines to reduce
    // false sharing between producer and consumer.
    alignas(64) std::atomic<size_t> head{0};
    alignas(64) std::atomic<size_t> tail{0};
};

// ------------------------------------------------------------
// Mutex queue benchmark.
// ------------------------------------------------------------
Result benchmark_mutex_queue() {
    MutexQueue queue;

    std::atomic<uint64_t> consumed{0};
    std::atomic<uint64_t> checksum{0};

    auto start = Clock::now();

    std::thread consumer([&]() {
        uint64_t value = 0;

        while (queue.pop(value)) {
            consumed.fetch_add(
                1,
                std::memory_order_relaxed
            );

            checksum.fetch_xor(
                value,
                std::memory_order_relaxed
            );
        }
    });

    std::thread producer([&]() {
        for (uint64_t i = 0; i < ITEM_COUNT; ++i) {
            queue.push(make_value(i));
        }

        queue.set_done();
    });

    producer.join();
    consumer.join();

    auto end = Clock::now();

    volatile uint64_t sink = checksum.load();
    (void)sink;

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    std::cout
        << "Mutex queue consumed: "
        << consumed.load()
        << "\n";

    return {
        elapsed_us,
        ITEM_COUNT / (elapsed_us / 1'000'000.0)
    };
}

// ------------------------------------------------------------
// SPSC ring buffer benchmark.
// ------------------------------------------------------------
Result benchmark_spsc_ring() {
    SPSCRingBuffer ring(RING_CAPACITY);

    std::atomic<uint64_t> consumed{0};
    std::atomic<uint64_t> checksum{0};

    auto start = Clock::now();

    std::thread consumer([&]() {
        uint64_t value = 0;

        while (consumed.load(std::memory_order_relaxed) < ITEM_COUNT) {
            if (ring.pop(value)) {
                consumed.fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                checksum.fetch_xor(
                    value,
                    std::memory_order_relaxed
                );
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread producer([&]() {
        for (uint64_t i = 0; i < ITEM_COUNT; ++i) {
            uint64_t value = make_value(i);

            while (!ring.push(value)) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = Clock::now();

    volatile uint64_t sink = checksum.load();
    (void)sink;

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    std::cout
        << "SPSC ring consumed: "
        << consumed.load()
        << "\n";

    return {
        elapsed_us,
        ITEM_COUNT / (elapsed_us / 1'000'000.0)
    };
}

void print_result(const char* name, const Result& result) {
    std::cout << name << "\n";
    std::cout << "Elapsed: " << result.elapsed_us << " us\n";
    std::cout << "Throughput: " << result.items_per_sec << " items/sec\n\n";
}

int main() {
    std::cout << "SPSC Ring Buffer vs Mutex Queue Benchmark\n";
    std::cout << "Items: " << ITEM_COUNT << "\n";
    std::cout << "Ring capacity: " << RING_CAPACITY << "\n\n";

    auto mutex_result = benchmark_mutex_queue();
    auto ring_result = benchmark_spsc_ring();

    print_result("Mutex queue", mutex_result);
    print_result("SPSC ring buffer", ring_result);

    std::cout
        << "SPSC ring / mutex queue: "
        << ring_result.items_per_sec / mutex_result.items_per_sec
        << "x\n";

    return 0;
}
