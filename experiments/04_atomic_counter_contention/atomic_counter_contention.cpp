#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Number of increments each thread performs.
constexpr uint64_t INCREMENTS_PER_THREAD = 10'000'000;

// Shared counter protected by mutex.
uint64_t mutex_counter = 0;

// Global mutex used by all threads in mutex test.
std::mutex counter_mutex;

// Shared atomic counter used by all threads in atomic test.
std::atomic<uint64_t> atomic_counter{0};

// Benchmark result.
struct Result {
    double throughput;
    long elapsed_us;
};

// ------------------------------------------------------------
// Mutex-protected shared counter benchmark.
//
// Every increment requires:
// 1. acquiring the mutex
// 2. incrementing shared state
// 3. releasing the mutex
//
// This intentionally creates heavy lock contention.
// ------------------------------------------------------------
Result run_mutex_counter(int thread_count) {
    mutex_counter = 0;

    std::vector<std::thread> threads;

    auto start = Clock::now();

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([]() {
            for (uint64_t i = 0; i < INCREMENTS_PER_THREAD; ++i) {
                std::lock_guard<std::mutex> lock(counter_mutex);
                ++mutex_counter;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total =
        static_cast<double>(INCREMENTS_PER_THREAD) * thread_count;

    return {
        total / (static_cast<double>(elapsed_us) / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Shared atomic counter benchmark.
//
// This avoids mutex locking, but all threads still update the
// same atomic variable.
//
// The atomic operation is relaxed, so it does not impose extra
// ordering constraints beyond atomicity. Even so, the cache line
// containing the counter still becomes shared hot state.
// ------------------------------------------------------------
Result run_atomic_counter(int thread_count) {
    atomic_counter.store(0, std::memory_order_relaxed);

    std::vector<std::thread> threads;

    auto start = Clock::now();

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([]() {
            for (uint64_t i = 0; i < INCREMENTS_PER_THREAD; ++i) {
                atomic_counter.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total =
        static_cast<double>(INCREMENTS_PER_THREAD) * thread_count;

    return {
        total / (static_cast<double>(elapsed_us) / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Per-thread local counter benchmark.
//
// Each thread updates private local state during the hot loop.
// There is no shared write in the main workload.
//
// Results are reduced after all threads finish.
// ------------------------------------------------------------
Result run_local_counters(int thread_count) {
    std::vector<uint64_t> local_counts(thread_count, 0);

    std::vector<std::thread> threads;

    auto start = Clock::now();

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t local = 0;

            for (uint64_t i = 0; i < INCREMENTS_PER_THREAD; ++i) {
                local += 1;

                // Small dependency chain prevents the compiler
                // from replacing the loop with a constant.
                local ^= (local << 7);
                local ^= (local >> 3);
            }

            local_counts[t] = local;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Reduction step after parallel work completes.
    uint64_t total_count = 0;

    for (auto value : local_counts) {
        total_count ^= value;
    }

    volatile uint64_t sink = total_count;
    (void)sink;

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total =
        static_cast<double>(INCREMENTS_PER_THREAD) * thread_count;

    return {
        total / (static_cast<double>(elapsed_us) / 1'000'000.0),
        elapsed_us
    };
}

int main() {
    std::vector<int> thread_counts = {
        1,
        2,
        4,
        6,
        8,
        12
    };

    std::cout << "Atomic Counter Contention Benchmark\n";

    std::cout
        << "Increments per thread: "
        << INCREMENTS_PER_THREAD
        << "\n\n";

    for (int threads : thread_counts) {
        auto mutex_result =
            run_mutex_counter(threads);

        auto atomic_result =
            run_atomic_counter(threads);

        auto local_result =
            run_local_counters(threads);

        std::cout
            << "Threads: "
            << threads
            << "\n";

        std::cout
            << "Mutex counter:  "
            << mutex_result.throughput
            << " inc/sec"
            << " (" << mutex_result.elapsed_us << " us)\n";

        std::cout
            << "Atomic counter: "
            << atomic_result.throughput
            << " inc/sec"
            << " (" << atomic_result.elapsed_us << " us)\n";

        std::cout
            << "Local counters: "
            << local_result.throughput
            << " inc/sec"
            << " (" << local_result.elapsed_us << " us)\n";

        std::cout
            << "Atomic / Mutex: "
            << atomic_result.throughput / mutex_result.throughput
            << "x\n";

        std::cout
            << "Local / Atomic: "
            << local_result.throughput / atomic_result.throughput
            << "x\n";

        std::cout << "-----------------------------\n";
    }

    return 0;
}
