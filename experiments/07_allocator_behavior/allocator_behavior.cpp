#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Number of objects handled per benchmark iteration.
constexpr int OBJECT_COUNT = 1'000'000;

// Number of repeated benchmark passes.
constexpr int ITERATIONS = 10;

// Small object type used by all allocation strategies.
struct Node {
    int value;
    Node* next;
};

// Benchmark result.
struct Result {
    double objects_per_sec;
    long elapsed_us;
};

// ------------------------------------------------------------
// new/delete benchmark.
//
// Allocates each Node individually with new and frees each one
// individually with delete.
//
// This represents per-object heap allocation in the hot path.
// ------------------------------------------------------------
Result benchmark_new_delete() {
    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Node*> nodes;
        nodes.reserve(OBJECT_COUNT);

        for (int i = 0; i < OBJECT_COUNT; ++i) {
            nodes.push_back(new Node{i, nullptr});
        }

        for (auto* node : nodes) {
            delete node;
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total_objects =
        static_cast<double>(OBJECT_COUNT) * ITERATIONS;

    return {
        total_objects / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// malloc/free benchmark.
//
// Allocates raw memory with malloc, constructs Node using
// placement new, manually calls destructor, and frees memory.
//
// Node has trivial destruction, but this keeps the benchmark
// structurally comparable to manual allocation patterns.
// ------------------------------------------------------------
Result benchmark_malloc_free() {
    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Node*> nodes;
        nodes.reserve(OBJECT_COUNT);

        for (int i = 0; i < OBJECT_COUNT; ++i) {
            void* mem = std::malloc(sizeof(Node));
            auto* node = new (mem) Node{i, nullptr};
            nodes.push_back(node);
        }

        for (auto* node : nodes) {
            node->~Node();
            std::free(node);
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total_objects =
        static_cast<double>(OBJECT_COUNT) * ITERATIONS;

    return {
        total_objects / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Preallocated vector benchmark.
//
// Uses contiguous storage and reserves capacity before pushing.
//
// This avoids per-object heap allocation and improves locality.
// ------------------------------------------------------------
Result benchmark_vector_preallocated() {
    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Node> nodes;
        nodes.reserve(OBJECT_COUNT);

        for (int i = 0; i < OBJECT_COUNT; ++i) {
            nodes.push_back(Node{i, nullptr});
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total_objects =
        static_cast<double>(OBJECT_COUNT) * ITERATIONS;

    return {
        total_objects / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// ------------------------------------------------------------
// Object pool benchmark.
//
// Allocates one pool once, then reuses objects by index.
//
// This avoids allocator calls inside each iteration.
// ------------------------------------------------------------
Result benchmark_object_pool() {
    std::vector<Node> pool(OBJECT_COUNT);

    auto start = Clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<Node*> nodes;
        nodes.reserve(OBJECT_COUNT);

        for (int i = 0; i < OBJECT_COUNT; ++i) {
            pool[i].value = i;
            pool[i].next = nullptr;
            nodes.push_back(&pool[i]);
        }

        // Simulate returning objects to the pool.
        for (auto* node : nodes) {
            node->value = 0;
            node->next = nullptr;
        }
    }

    auto end = Clock::now();

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    double total_objects =
        static_cast<double>(OBJECT_COUNT) * ITERATIONS;

    return {
        total_objects / (elapsed_us / 1'000'000.0),
        elapsed_us
    };
}

// Prints one benchmark result.
void print_result(const char* name, const Result& result) {
    std::cout << name << "\n";
    std::cout << "Elapsed: " << result.elapsed_us << " us\n";
    std::cout
        << "Throughput: "
        << result.objects_per_sec
        << " objects/sec\n\n";
}

int main() {
    std::cout << "Allocator Behavior Benchmark\n";
    std::cout << "Objects per iteration: " << OBJECT_COUNT << "\n";
    std::cout << "Iterations: " << ITERATIONS << "\n\n";

    auto new_delete_result = benchmark_new_delete();
    auto malloc_free_result = benchmark_malloc_free();
    auto vector_result = benchmark_vector_preallocated();
    auto pool_result = benchmark_object_pool();

    print_result("new/delete", new_delete_result);
    print_result("malloc/free", malloc_free_result);
    print_result("vector preallocated", vector_result);
    print_result("object pool reuse", pool_result);

    std::cout << "object pool / new-delete: "
              << pool_result.objects_per_sec
                   / new_delete_result.objects_per_sec
              << "x\n";

    std::cout << "vector / new-delete: "
              << vector_result.objects_per_sec
                   / new_delete_result.objects_per_sec
              << "x\n";

    return 0;
}
