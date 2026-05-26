# Experiment 01 - Multithread Scaling Baseline

Measures CPU-bound throughput scaling across increasing thread counts on a Ryzen 5 5600X system running Linux via WSL2.

## Goal

Establish a baseline for multicore scaling behavior before introducing contention, shared state, false sharing, locks, allocators, or scheduler constraints.

This experiment measures how throughput changes across:

- 1 thread
- 2 threads
- 4 threads
- 6 threads
- 8 threads
- 12 threads

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
- `perf` 7.0.0

## Method

Each thread runs an independent CPU-bound integer workload.

The benchmark keeps work per thread constant:

- 200,000,000 iterations per thread

Total work increases with thread count.

Throughput is calculated as:

    total iterations / elapsed time

This creates a scaling baseline showing how well independent CPU work scales as more hardware threads are used.

## Results

### Native Run

| Threads | Elapsed | Throughput | Speedup |
|---|---:|---:|---:|
| 1 | 304 ms | 6.57895e+08 iter/s | 1.00x |
| 2 | 304 ms | 1.31579e+09 iter/s | 2.00x |
| 4 | 306 ms | 2.61438e+09 iter/s | 3.97x |
| 6 | 310 ms | 3.87097e+09 iter/s | 5.88x |
| 8 | 314 ms | 5.09554e+09 iter/s | 7.75x |
| 12 | 400 ms | 6.00000e+09 iter/s | 9.12x |

### perf Run

| Threads | Elapsed | Throughput | Speedup |
|---|---:|---:|---:|
| 1 | 307 ms | 6.51466e+08 iter/s | 1.00x |
| 2 | 313 ms | 1.27796e+09 iter/s | 1.96x |
| 4 | 312 ms | 2.56410e+09 iter/s | 3.94x |
| 6 | 313 ms | 3.83387e+09 iter/s | 5.88x |
| 8 | 319 ms | 5.01567e+09 iter/s | 7.70x |
| 12 | 352 ms | 6.81818e+09 iter/s | 10.47x |

## perf Summary

    10.52271 sec task-clock
    5.4 CPUs utilized
    178 page-faults
    47,180,262,402 cpu-cycles
    59,364,681,062 instructions
    1.3 instructions per cycle
    1.921047202 seconds elapsed
    10.441088000 seconds user
    0.011937000 seconds sys

## Interpretation

The benchmark shows near-linear scaling through the physical core count.

From 1 to 6 threads, speedup reaches approximately 5.88x, which is close to ideal scaling on a 6-core CPU.

At 8 and 12 threads, throughput continues improving, but scaling becomes less efficient. This reflects the transition from using physical cores to also using SMT hardware threads.

The 12-thread result still improves throughput, but not by a full 12x. This indicates that SMT contributes additional execution capacity, but does not behave like additional full physical cores.

The `perf` run also establishes an initial low-level profile:

- high user time
- minimal system time
- low page fault count
- no visible context-switch pressure
- IPC around 1.3

This creates a clean baseline before introducing shared state, lock contention, false sharing, or allocator pressure.

## Why This Matters

This is the Phase 2 baseline experiment.

Before analyzing bottlenecks, contention, or cache effects, the repo first establishes how independent CPU-bound work scales across the available hardware threads.

Future experiments can compare against this baseline to show how different performance pathologies reduce scalability.

