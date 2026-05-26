#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

// Monotonic clock used for benchmark timing.
using Clock = std::chrono::steady_clock;

// Fixed amount of CPU work assigned to each thread.
//
// Work per thread stays constant, so total work increases
// as thread count increases.
constexpr uint64_t WORK_PER_THREAD = 200'000'000;

// ------------------------------------------------------------
// CPU-bound synthetic workload.
//
// This intentionally performs integer arithmetic with a
// dependency chain so the compiler cannot trivially remove
// or vectorize the entire loop away.
//
// The exact computation is not meaningful. It exists to keep
// CPU cores busy with repeatable work.
// ------------------------------------------------------------
uint64_t compute_work(uint64_t iterations) {
    uint64_t value = 0x12345678;

    for (uint64_t i = 0; i < iterations; ++i) {
        value ^= i;
        value *= 1664525;
        value += 1013904223;
        value ^= value >> 13;
    }

    return value;
}

// ------------------------------------------------------------
// Runs benchmark for one thread count.
// ------------------------------------------------------------
double run_benchmark(int thread_count) {
    std::vector<std::thread> threads;

    // One result slot per thread.
    //
    // Results are collected so each thread's work has an
    // observable output.
    std::vector<uint64_t> results(thread_count);

    auto start = Clock::now();

    // --------------------------------------------------------
    // Launch worker threads.
    // --------------------------------------------------------
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            results[t] =
                compute_work(WORK_PER_THREAD);
        });
    }

    // --------------------------------------------------------
    // Wait for all threads to finish.
    // --------------------------------------------------------
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    // --------------------------------------------------------
    // Consume results so compiler cannot discard thread work.
    // --------------------------------------------------------
    volatile uint64_t sink = 0;

    for (auto value : results) {
        sink ^= value;
    }

    auto elapsed_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                end - start
            ).count();

    double total_work =
        static_cast<double>(WORK_PER_THREAD)
            * thread_count;

    double throughput =
        total_work
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
        << "Throughput: "
        << throughput
        << " iterations/sec\n\n";

    return throughput;
}

int main() {
    // Thread counts chosen to observe:
    //
    // - single-thread baseline
    // - partial physical core usage
    // - full physical core usage
    // - SMT / logical CPU scaling
    std::vector<int> thread_counts = {
        1,
        2,
        4,
        6,
        8,
        12
    };

    std::cout << "Multithread Scaling Baseline\n";
    std::cout
        << "Work per thread: "
        << WORK_PER_THREAD
        << "\n\n";

    double baseline = 0.0;

    for (int threads : thread_counts) {
        double throughput =
            run_benchmark(threads);

        if (threads == 1) {
            baseline = throughput;
        }

        std::cout
            << "Speedup vs 1 thread: "
            << throughput / baseline
            << "x\n";

        std::cout
            << "-----------------------------\n";
    }

    return 0;
}
