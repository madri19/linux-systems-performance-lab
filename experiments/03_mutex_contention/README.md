# Experiment 03 - Mutex Contention

Measures how throughput changes when multiple threads contend for a single mutex-protected shared counter.

## Goal

Show how lock contention can destroy scalability even when more CPU threads are available.

This experiment compares throughput across:

- 1 thread
- 2 threads
- 4 threads
- 6 threads
- 8 threads
- 12 threads

Each thread repeatedly increments the same shared counter while holding a mutex.

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

    5,000,000 increments

Every increment is protected by:

    std::mutex
    std::lock_guard

All threads contend for the same mutex and update the same shared counter.

Throughput is calculated as:

    total increments / elapsed time

## Results

### Native Run

| Threads | Elapsed | Counter | Throughput | Throughput vs 1 Thread |
|---|---:|---:|---:|---:|
| 1 | 25 ms | 5,000,000 | 2.00000e+08 inc/s | 1.00x |
| 2 | 201 ms | 10,000,000 | 4.97512e+07 inc/s | 0.25x |
| 4 | 365 ms | 20,000,000 | 5.47945e+07 inc/s | 0.27x |
| 6 | 639 ms | 30,000,000 | 4.69484e+07 inc/s | 0.23x |
| 8 | 1014 ms | 40,000,000 | 3.94477e+07 inc/s | 0.20x |
| 12 | 1559 ms | 60,000,000 | 3.84862e+07 inc/s | 0.19x |

### perf Run

| Threads | Elapsed | Counter | Throughput | Throughput vs 1 Thread |
|---|---:|---:|---:|---:|
| 1 | 25 ms | 5,000,000 | 2.00000e+08 inc/s | 1.00x |
| 2 | 193 ms | 10,000,000 | 5.18135e+07 inc/s | 0.26x |
| 4 | 269 ms | 20,000,000 | 7.43494e+07 inc/s | 0.37x |
| 6 | 402 ms | 30,000,000 | 7.46269e+07 inc/s | 0.37x |
| 8 | 622 ms | 40,000,000 | 6.43087e+07 inc/s | 0.32x |
| 12 | 1319 ms | 60,000,000 | 4.54890e+07 inc/s | 0.23x |

## perf Summary

    20338.03 msec task-clock
    7.1 CPUs utilized
    177 page-faults
    11,182,802,739 cpu-cycles
    11,448,830,063 instructions
    1.0 instructions per cycle
    2.835681518 seconds elapsed
    3.441925000 seconds user
    13.545378000 seconds sys

## Interpretation

Adding threads made total throughput worse instead of better.

The 1-thread baseline reached approximately:

    200 million increments/sec

The 12-thread run reached only:

    38-45 million increments/sec

This means the 12-thread case produced roughly 4-5x less throughput than the single-thread case.

The reason is that every increment serializes through one mutex. More threads do not increase useful parallelism. Instead, they increase contention.

The perf result is especially important:

    user time: 3.44s
    system time: 13.55s

Most time was spent in system/kernel overhead rather than useful application work.

## Why This Matters

This experiment demonstrates a core systems-performance lesson:

More threads do not automatically mean more throughput.

When work is serialized behind a shared lock, additional threads can reduce performance by increasing:

- lock contention
- scheduler overhead
- kernel time
- cache-line ownership traffic
- synchronization pressure

This builds on the first two Phase 2 experiments:

- Experiment 01 showed independent work scaling across cores
- Experiment 02 showed cache-line false sharing destroying performance
- Experiment 03 shows mutex contention destroying scalability

