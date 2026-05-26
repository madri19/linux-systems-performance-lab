#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Number of increments performed by each thread.
constexpr uint64_t INCREMENTS_PER_THREAD = 5'000'000;

// Shared mutex protecting the counter.
//
// All threads contend for this same lock.
std::mutex counter_mutex;

// Shared counter updated by all threads.
uint64_t shared_counter = 0;

// ------------------------------------------------------------
// Runs one benchmark for a specific thread count.
//
// Each thread repeatedly:
// 1. locks the mutex
// 2. increments the shared counter
// 3. unlocks the mutex
//
// This intentionally creates heavy lock contention.
// ------------------------------------------------------------
double run_benchmark(int thread_count) {

    // Reset shared state before each run.
    shared_counter = 0;

    std::vector<std::thread> threads;

    auto start = Clock::now();

    // --------------------------------------------------------
    // Launch worker threads.
    // --------------------------------------------------------
    for (int t = 0; t < thread_count; ++t) {

        threads.emplace_back([]() {

            for (uint64_t i = 0;
                 i < INCREMENTS_PER_THREAD;
                 ++i) {

                // lock_guard locks on construction and unlocks
                // automatically when it leaves scope.
                std::lock_guard<std::mutex> lock(
                    counter_mutex
                );

                ++shared_counter;
            }
        });
    }

    // --------------------------------------------------------
    // Wait for all threads to finish.
    // --------------------------------------------------------
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                end - start
            ).count();

    double total_increments =
        static_cast<double>(
            INCREMENTS_PER_THREAD
        ) * thread_count;

    double throughput =
        total_increments
            / (static_cast<double>(elapsed_ms) / 1000.0);

    std::cout
        << "Threads: "
        << thread_count
        << "\n";

    std::cout
        << "Elapsed: "
        << elapsed_ms
        << " ms\n";

    std::cout
        << "Counter: "
        << shared_counter
        << "\n";

    std::cout
        << "Throughput: "
        << throughput
        << " increments/sec\n\n";

    return throughput;
}

int main() {

    // Thread counts chosen to compare:
    //
    // - single-thread baseline
    // - increasing lock contention
    // - full physical core count
    // - SMT/logical CPU contention
    std::vector<int> thread_counts = {
        1,
        2,
        4,
        6,
        8,
        12
    };

    std::cout << "Mutex Contention Benchmark\n";

    std::cout
        << "Increments per thread: "
        << INCREMENTS_PER_THREAD
        << "\n\n";

    double baseline = 0.0;

    for (int threads : thread_counts) {

        double throughput =
            run_benchmark(threads);

        if (threads == 1) {
            baseline = throughput;
        }

        std::cout
            << "Throughput vs 1 thread: "
            << throughput / baseline
            << "x\n";

        std::cout
            << "-----------------------------\n";
    }

    return 0;
}
