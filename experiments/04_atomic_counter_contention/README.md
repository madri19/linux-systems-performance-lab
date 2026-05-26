# Experiment 04 - Atomic Counter Contention

Compares three counter update strategies across increasing thread counts on a multicore Ryzen system.

## Goal

Show that atomics avoid mutex locking overhead, but a shared atomic counter can still become a scalability bottleneck due to cache-coherence contention.

This experiment compares:

- mutex-protected shared counter
- shared atomic counter
- per-thread local counters reduced at the end

## Hardware

### CPU

- AMD Ryzen 5 5600X
- 6 physical cores
- 12 logical CPUs / SMT threads
- 32MB L3 cache

### Environment

- Ubuntu on WSL2
- Linux 6.6.114.1-microsoft-standard-WSL2
- x86_64
- GCC 15.2.0
- perf 7.0.0

## Method

Each thread performs:

    10,000,000 increments

Three strategies are measured.

### Mutex Counter

All threads update one shared counter protected by `std::mutex`.

### Atomic Counter

All threads update one shared `std::atomic<uint64_t>` counter using relaxed atomic increments.

### Local Counters

Each thread updates a private local counter during the hot loop.

After all threads finish, local results are reduced once.

## Results

### Native Run

| Threads | Mutex Counter | Atomic Counter | Local Counters |
|---|---:|---:|---:|
| 1 | 1.97855e+08 inc/s | 6.42220e+08 inc/s | 9.14160e+08 inc/s |
| 2 | 6.23208e+07 inc/s | 1.26473e+08 inc/s | 1.76382e+09 inc/s |
| 4 | 6.15242e+07 inc/s | 1.05623e+08 inc/s | 3.48857e+09 inc/s |
| 6 | 5.66203e+07 inc/s | 1.04770e+08 inc/s | 5.00292e+09 inc/s |
| 8 | 4.83935e+07 inc/s | 1.00596e+08 inc/s | 6.49562e+09 inc/s |
| 12 | 4.62029e+07 inc/s | 9.73963e+07 inc/s | 7.95756e+09 inc/s |

### perf Run

| Threads | Mutex Counter | Atomic Counter | Local Counters |
|---|---:|---:|---:|
| 1 | 1.95271e+08 inc/s | 6.28101e+08 inc/s | 8.91424e+08 inc/s |
| 2 | 5.81874e+07 inc/s | 1.27146e+08 inc/s | 1.75055e+09 inc/s |
| 4 | 7.24747e+07 inc/s | 1.04950e+08 inc/s | 3.47313e+09 inc/s |
| 6 | 7.23904e+07 inc/s | 1.03672e+08 inc/s | 4.95622e+09 inc/s |
| 8 | 6.64035e+07 inc/s | 1.02574e+08 inc/s | 6.32661e+09 inc/s |
| 12 | 4.53395e+07 inc/s | 9.39378e+07 inc/s | 6.07380e+09 inc/s |

## perf Summary

    68898.60 msec task-clock
    7.7 CPUs utilized
    233 page-faults
    131,630,312,545 cpu-cycles
    27,470,030,474 instructions
    0.2 instructions per cycle
    8.898466794 seconds elapsed
    33.755943000 seconds user
    28.206575000 seconds sys

## Interpretation

The shared atomic counter was faster than the mutex-protected counter, but both scaled poorly.

At 12 threads:

    Mutex counter:  ~45M inc/sec
    Atomic counter: ~94-97M inc/sec
    Local counters: ~6-8B inc/sec

The atomic counter avoids mutex locking, but it still forces all threads to repeatedly update the same cache line.

That means the bottleneck moves from lock contention to cache-coherence contention.

The local-counter approach performs dramatically better because threads avoid shared writes during the hot loop. Each thread works independently, and the final reduction is cheap compared to constant synchronization.

## Key Lesson

The hierarchy observed here is:

    mutex contention < shared atomic contention << local aggregation

Atomics are not magic scalability tools.

They remove some locking overhead, but shared atomic writes can still serialize performance through cache-line ownership and coherence traffic.

## Why This Matters

This experiment bridges mutex contention and lock-free thinking.

It shows that improving synchronization requires more than replacing mutexes with atomics.

Good scalable designs often avoid shared hot-state entirely by using:

- per-thread counters
- batching
- sharding
- reduction
- ownership partitioning
- cache-aware data layout

