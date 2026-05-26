#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Monotonic clock for benchmark timing.
using Clock = std::chrono::steady_clock;

// Large-batch scenario.
constexpr int LARGE_BATCH_TASKS = 10000;

// Small-batch scenario.
constexpr int SMALL_BATCHES = 1000;
constexpr int TASKS_PER_SMALL_BATCH = 24;

// CPU work performed by each task.
constexpr int WORK_PER_TASK = 10000;

// Worker count chosen to match physical core count.
constexpr int THREAD_COUNT = 6;

// ------------------------------------------------------------
// Synthetic CPU-bound task.
//
// The computation is arbitrary. It exists to give each task
// repeatable work that cannot be trivially optimized away.
// ------------------------------------------------------------
uint64_t do_work(int seed) {
    uint64_t value =
        static_cast<uint64_t>(seed) + 0x12345678;

    for (int i = 0; i < WORK_PER_TASK; ++i) {
        value ^= i;
        value *= 1664525;
        value += 1013904223;
        value ^= value >> 13;
    }

    return value;
}

// Benchmark result.
struct Result {
    long elapsed_us;
    double tasks_per_sec;
};

// ------------------------------------------------------------
// Minimal thread pool implementation.
//
// Workers wait on a condition variable, pull tasks from a queue,
// execute them, and notify when all queued/active work is done.
// ------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(int thread_count) {
        for (int i = 0; i < thread_count; ++i) {
            workers.emplace_back([this]() {
                worker_loop();
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }

        cv.notify_all();

        for (auto& worker : workers) {
            worker.join();
        }
    }

    // Submit one task to the queue.
    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push_back(std::move(task));
        }

        cv.notify_one();
    }

    // Wait until the queue is empty and no worker is active.
    void wait_until_done() {
        std::unique_lock<std::mutex> lock(mutex);

        done_cv.wait(lock, [this]() {
            return tasks.empty() && active_tasks == 0;
        });
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(mutex);

                cv.wait(lock, [this]() {
                    return stopping || !tasks.empty();
                });

                if (stopping && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop_front();
                ++active_tasks;
            }

            task();

            {
                std::lock_guard<std::mutex> lock(mutex);

                --active_tasks;

                if (tasks.empty() && active_tasks == 0) {
                    done_cv.notify_one();
                }
            }
        }
    }

    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable cv;
    std::condition_variable done_cv;

    bool stopping = false;
    int active_tasks = 0;
};

// ------------------------------------------------------------
// Spawns THREAD_COUNT workers for one batch of tasks.
//
// Workers pull task IDs from an atomic counter.
// ------------------------------------------------------------
Result spawn_worker_batch(int task_count) {
    std::atomic<int> next_task{0};
    std::vector<uint64_t> results(task_count);
    std::vector<std::thread> threads;

    auto start = Clock::now();

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            while (true) {
                int task_id =
                    next_task.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );

                if (task_id >= task_count) {
                    break;
                }

                results[task_id] =
                    do_work(task_id);
            }
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

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    return {
        elapsed_us,
        task_count / (elapsed_us / 1'000'000.0)
    };
}

// ------------------------------------------------------------
// Uses an existing thread pool for one batch of tasks.
// ------------------------------------------------------------
Result pool_worker_batch(ThreadPool& pool, int task_count) {
    std::vector<uint64_t> results(task_count);

    auto start = Clock::now();

    for (int task_id = 0; task_id < task_count; ++task_id) {
        pool.submit([&, task_id]() {
            results[task_id] =
                do_work(task_id);
        });
    }

    pool.wait_until_done();

    auto end = Clock::now();

    volatile uint64_t sink = 0;

    for (auto value : results) {
        sink ^= value;
    }

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    return {
        elapsed_us,
        task_count / (elapsed_us / 1'000'000.0)
    };
}

// One large batch: spawn workers once.
Result scenario_large_batch_spawn() {
    return spawn_worker_batch(LARGE_BATCH_TASKS);
}

// One large batch: use thread pool once.
Result scenario_large_batch_pool() {
    ThreadPool pool(THREAD_COUNT);
    return pool_worker_batch(pool, LARGE_BATCH_TASKS);
}

// Many small batches: repeatedly create/destroy workers.
Result scenario_many_small_batches_spawn() {
    auto start = Clock::now();

    for (int batch = 0; batch < SMALL_BATCHES; ++batch) {
        auto result =
            spawn_worker_batch(TASKS_PER_SMALL_BATCH);

        (void)result;
    }

    auto end = Clock::now();

    int total_tasks =
        SMALL_BATCHES * TASKS_PER_SMALL_BATCH;

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    return {
        elapsed_us,
        total_tasks / (elapsed_us / 1'000'000.0)
    };
}

// Many small batches: reuse persistent workers.
Result scenario_many_small_batches_pool() {
    ThreadPool pool(THREAD_COUNT);

    auto start = Clock::now();

    for (int batch = 0; batch < SMALL_BATCHES; ++batch) {
        auto result =
            pool_worker_batch(
                pool,
                TASKS_PER_SMALL_BATCH
            );

        (void)result;
    }

    auto end = Clock::now();

    int total_tasks =
        SMALL_BATCHES * TASKS_PER_SMALL_BATCH;

    auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();

    return {
        elapsed_us,
        total_tasks / (elapsed_us / 1'000'000.0)
    };
}

void print_result(const char* name, const Result& result) {
    std::cout << name << "\n";

    std::cout
        << "Elapsed: "
        << result.elapsed_us
        << " us\n";

    std::cout
        << "Throughput: "
        << result.tasks_per_sec
        << " tasks/sec\n\n";
}

int main() {
    std::cout << "Thread Pool vs Thread Spawning Benchmark\n";
    std::cout << "Worker threads: " << THREAD_COUNT << "\n";
    std::cout << "Work per task: " << WORK_PER_TASK << "\n\n";

    std::cout << "Scenario A: one large batch\n";
    std::cout << "Tasks: " << LARGE_BATCH_TASKS << "\n\n";

    auto large_spawn =
        scenario_large_batch_spawn();

    auto large_pool =
        scenario_large_batch_pool();

    print_result("Spawn workers once", large_spawn);
    print_result("Thread pool", large_pool);

    std::cout
        << "Thread pool / spawn: "
        << large_pool.tasks_per_sec
             / large_spawn.tasks_per_sec
        << "x\n";

    std::cout << "-----------------------------\n\n";

    std::cout << "Scenario B: many small batches\n";
    std::cout << "Batches: " << SMALL_BATCHES << "\n";
    std::cout
        << "Tasks per batch: "
        << TASKS_PER_SMALL_BATCH
        << "\n\n";

    auto small_spawn =
        scenario_many_small_batches_spawn();

    auto small_pool =
        scenario_many_small_batches_pool();

    print_result("Spawn workers every batch", small_spawn);
    print_result("Thread pool reused", small_pool);

    std::cout
        << "Thread pool / spawn: "
        << small_pool.tasks_per_sec
             / small_spawn.tasks_per_sec
        << "x\n";

    return 0;
}
