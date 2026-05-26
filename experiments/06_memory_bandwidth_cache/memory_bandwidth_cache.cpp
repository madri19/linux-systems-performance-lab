#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Number of passes over each working set.
constexpr int ITERATIONS = 20;

// Benchmark result.
struct Result {
    double mb_per_sec;
    long elapsed_us;
};

// ------------------------------------------------------------
// Sequential read benchmark.
//
// Reads the array linearly from beginning to end.
//
// This access pattern is cache-friendly and gives the CPU's
// hardware prefetchers a predictable stream.
// ------------------------------------------------------------
Result sequential_read(const std::vector<uint64_t>& data) {
    volatile uint64_t sink = 0;

    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (size_t i = 0; i < data.size(); ++i) {
            sink += data[i];
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double bytes =
        static_cast<double>(data.size() * sizeof(uint64_t)) * ITERATIONS;

    return {
        bytes / (1024.0 * 1024.0) / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Sequential write benchmark.
//
// Writes the array linearly from beginning to end.
// ------------------------------------------------------------
Result sequential_write(std::vector<uint64_t>& data) {
    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<uint64_t>(i + iter);
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double bytes =
        static_cast<double>(data.size() * sizeof(uint64_t)) * ITERATIONS;

    return {
        bytes / (1024.0 * 1024.0) / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Random read benchmark.
//
// Reads the same array using shuffled indices.
//
// This destroys predictable locality and makes hardware
// prefetching much less effective.
// ------------------------------------------------------------
Result random_read(
    const std::vector<uint64_t>& data,
    const std::vector<size_t>& indices
) {
    volatile uint64_t sink = 0;

    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (size_t index : indices) {
            sink += data[index];
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double bytes =
        static_cast<double>(data.size() * sizeof(uint64_t)) * ITERATIONS;

    return {
        bytes / (1024.0 * 1024.0) / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Random write benchmark.
//
// Writes the same array using shuffled indices.
// ------------------------------------------------------------
Result random_write(
    std::vector<uint64_t>& data,
    const std::vector<size_t>& indices
) {
    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (size_t index : indices) {
            data[index] = static_cast<uint64_t>(index + iter);
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double bytes =
        static_cast<double>(data.size() * sizeof(uint64_t)) * ITERATIONS;

    return {
        bytes / (1024.0 * 1024.0) / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Runs all access-pattern benchmarks for one working-set size.
// ------------------------------------------------------------
void run_size(size_t bytes) {
    size_t elements = bytes / sizeof(uint64_t);

    std::vector<uint64_t> data(elements);
    std::iota(data.begin(), data.end(), 1);

    std::vector<size_t> indices(elements);
    std::iota(indices.begin(), indices.end(), 0);

    // Fixed seed for reproducibility.
    std::mt19937 rng(42);

    std::shuffle(
        indices.begin(),
        indices.end(),
        rng
    );

    auto seq_read = sequential_read(data);
    auto seq_write = sequential_write(data);
    auto rand_read = random_read(data, indices);
    auto rand_write = random_write(data, indices);

    std::cout << "Working set: " << bytes / 1024 << " KiB\n";
    std::cout << "Elements: " << elements << "\n";

    std::cout << "Sequential read:  "
              << seq_read.mb_per_sec
              << " MiB/s"
              << " (" << seq_read.elapsed_us << " us)\n";

    std::cout << "Sequential write: "
              << seq_write.mb_per_sec
              << " MiB/s"
              << " (" << seq_write.elapsed_us << " us)\n";

    std::cout << "Random read:      "
              << rand_read.mb_per_sec
              << " MiB/s"
              << " (" << rand_read.elapsed_us << " us)\n";

    std::cout << "Random write:     "
              << rand_write.mb_per_sec
              << " MiB/s"
              << " (" << rand_write.elapsed_us << " us)\n";

    std::cout << "Random read / Sequential read: "
              << rand_read.mb_per_sec / seq_read.mb_per_sec
              << "x\n";

    std::cout << "-----------------------------\n";
}

int main() {
    std::cout << "Memory Bandwidth and Cache Locality Benchmark\n";
    std::cout << "Iterations per working set: " << ITERATIONS << "\n\n";

    // Working sets selected to cross rough cache-size regions:
    //
    // 32 KiB   - small cache-friendly region
    // 256 KiB  - larger private-cache-sized region
    // 4 MiB    - larger shared-cache region
    // 32 MiB   - near L3 cache size on this CPU
    // 128 MiB  - beyond cache, main-memory dominated
    std::vector<size_t> working_sets = {
        32 * 1024,
        256 * 1024,
        4 * 1024 * 1024,
        32 * 1024 * 1024,
        128 * 1024 * 1024
    };

    for (size_t bytes : working_sets) {
        run_size(bytes);
    }

    return 0;
}
