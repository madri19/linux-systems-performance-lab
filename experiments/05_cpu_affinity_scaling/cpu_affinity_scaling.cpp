#include <chrono>
#include <cstdint>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Fixed CPU work per thread.
constexpr uint64_t WORK_PER_THREAD = 200'000'000;

// ------------------------------------------------------------
// CPU-bound synthetic workload.
//
// Same style as Experiment 01.
// Independent arithmetic keeps each thread busy without shared
// synchronization.
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
// Pins the current thread to one logical CPU.
//
// pthread_setaffinity_np is Linux-specific.
// ------------------------------------------------------------
void pin_current_thread_to_cpu(int cpu_id) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &cpuset
    );
}

// ------------------------------------------------------------
// Runs benchmark without manual CPU affinity.
//
// Linux scheduler decides where threads run.
// ------------------------------------------------------------
double run_unpinned(int thread_count) {
    std::vector<std::thread> threads;
    std::vector<uint64_t> results(thread_count);

    auto start = Clock::now();

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            results[t] =
                compute_work(WORK_PER_THREAD);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

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

    return total_work
        / (static_cast<double>(elapsed_ms) / 1000.0);
}

// ------------------------------------------------------------
// Runs benchmark with manual CPU affinity.
//
// Threads are pinned to CPU IDs from cpu_ids.
// If thread_count exceeds cpu_ids.size(), assignment wraps.
// ------------------------------------------------------------
double run_pinned(
    int thread_count,
    const std::vector<int>& cpu_ids
) {
    std::vector<std::thread> threads;
    std::vector<uint64_t> results(thread_count);

    auto start = Clock::now();

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            int cpu_id =
                cpu_ids[t % cpu_ids.size()];

            pin_current_thread_to_cpu(cpu_id);

            results[t] =
                compute_work(WORK_PER_THREAD);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

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

    return total_work
        / (static_cast<double>(elapsed_ms) / 1000.0);
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

    // All logical CPUs exposed by WSL2 on this machine.
    std::vector<int> first_logical_cpus = {
        0, 1, 2, 3, 4, 5,
        6, 7, 8, 9, 10, 11
    };

    // Smaller spaced CPU set.
    //
    // This behaves well up to 6 threads, but intentionally
    // underuses the machine above 6 threads.
    std::vector<int> spaced_cpus = {
        0, 2, 4, 6, 8, 10
    };

    std::cout << "CPU Affinity Scaling Benchmark\n";

    std::cout
        << "Work per thread: "
        << WORK_PER_THREAD
        << "\n\n";

    for (int threads : thread_counts) {
        double unpinned =
            run_unpinned(threads);

        double pinned_logical =
            run_pinned(
                threads,
                first_logical_cpus
            );

        double pinned_spaced =
            run_pinned(
                threads,
                spaced_cpus
            );

        std::cout
            << "Threads: "
            << threads
            << "\n";

        std::cout
            << "Unpinned:        "
            << unpinned
            << " iter/sec\n";

        std::cout
            << "Pinned logical:  "
            << pinned_logical
            << " iter/sec\n";

        std::cout
            << "Pinned spaced:   "
            << pinned_spaced
            << " iter/sec\n";

        std::cout
            << "Logical / Unpinned: "
            << pinned_logical / unpinned
            << "x\n";

        std::cout
            << "Spaced / Unpinned:  "
            << pinned_spaced / unpinned
            << "x\n";

        std::cout << "-----------------------------\n";
    }

    return 0;
}
