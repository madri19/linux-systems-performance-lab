#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;

constexpr uint64_t ITERATIONS = 200'000'000;

// ------------------------------------------------------------
// False-sharing layout.
//
// These counters are separate variables, but they are adjacent
// in memory. On most CPUs, both will likely live inside the
// same 64-byte cache line.
//
// If two threads on different cores repeatedly update these
// counters, the cache line must move between cores even though
// the variables are logically independent.
// ------------------------------------------------------------
struct SharedCounters {
    std::atomic<uint64_t> a{0};
    std::atomic<uint64_t> b{0};
};

// ------------------------------------------------------------
// Padded counter.
//
// alignas(64) forces the object to start on a cache-line-sized
// boundary on typical x86_64 systems.
//
// Padding prevents two hot counters from occupying the same
// cache line.
// ------------------------------------------------------------
struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value{0};
};

// ------------------------------------------------------------
// Padded layout.
//
// Each counter is isolated onto its own cache line.
// ------------------------------------------------------------
struct PaddedCounters {
    PaddedCounter a;
    PaddedCounter b;
};

// ------------------------------------------------------------
// Runs the false-sharing version.
//
// Thread 1 increments counter a.
// Thread 2 increments counter b.
//
// The counters are logically independent but likely share a
// cache line.
// ------------------------------------------------------------
long run_false_sharing() {
    SharedCounters counters;

    auto start = Clock::now();

    std::thread t1([&]() {
        for (uint64_t i = 0; i < ITERATIONS; ++i) {
            counters.a.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    });

    std::thread t2([&]() {
        for (uint64_t i = 0; i < ITERATIONS; ++i) {
            counters.b.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    });

    t1.join();
    t2.join();

    auto end = Clock::now();

    volatile uint64_t sink =
        counters.a.load() + counters.b.load();

    (void)sink;

    return std::chrono::duration_cast<
        std::chrono::milliseconds>(
            end - start
        ).count();
}

// ------------------------------------------------------------
// Runs the padded version.
//
// Same logical workload as false-sharing version, but counters
// are separated by cache-line alignment.
// ------------------------------------------------------------
long run_padded() {
    PaddedCounters counters;

    auto start = Clock::now();

    std::thread t1([&]() {
        for (uint64_t i = 0; i < ITERATIONS; ++i) {
            counters.a.value.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    });

    std::thread t2([&]() {
        for (uint64_t i = 0; i < ITERATIONS; ++i) {
            counters.b.value.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    });

    t1.join();
    t2.join();

    auto end = Clock::now();

    volatile uint64_t sink =
        counters.a.value.load() + counters.b.value.load();

    (void)sink;

    return std::chrono::duration_cast<
        std::chrono::milliseconds>(
            end - start
        ).count();
}

int main() {
    std::cout << "False Sharing Benchmark\n";
    std::cout
        << "Iterations per thread: "
        << ITERATIONS
        << "\n\n";

    auto false_sharing_ms = run_false_sharing();
    auto padded_ms = run_padded();

    std::cout
        << "False sharing layout: "
        << false_sharing_ms
        << " ms\n";

    std::cout
        << "Padded layout:        "
        << padded_ms
        << " ms\n\n";

    std::cout
        << "Speedup from padding: "
        << static_cast<double>(false_sharing_ms) / padded_ms
        << "x\n";

    return 0;
}
