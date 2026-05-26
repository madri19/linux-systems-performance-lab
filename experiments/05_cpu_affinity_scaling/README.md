# Experiment 05 - CPU Affinity Scaling

Compares default scheduler placement against manually pinned CPU affinity strategies on a Ryzen 5 5600X system.

## Goal

Investigate whether pinning threads to specific logical CPUs improves or hurts throughput.

This experiment compares:

- unpinned threads
- threads pinned across logical CPUs 0-11
- threads pinned to a smaller spaced CPU set

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

Each thread runs an independent CPU-bound integer workload.

Work per thread:

    200,000,000 iterations

Thread counts tested:

- 1
- 2
- 4
- 6
- 8
- 12

Three placement strategies are compared.

### Unpinned

Threads are created normally and left to the Linux scheduler.

### Pinned Logical

Threads are pinned across logical CPUs:

    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11

### Pinned Spaced

Threads are pinned across a smaller spaced CPU set:

    0, 2, 4, 6, 8, 10

For thread counts above 6, this causes multiple threads to share the same pinned CPU IDs.

## Results

### Native Run

| Threads | Unpinned | Pinned Logical | Pinned Spaced | Logical / Unpinned | Spaced / Unpinned |
|---|---:|---:|---:|---:|---:|
| 1 | 6.57895e+08 | 6.57895e+08 | 6.47249e+08 | 1.00x | 0.98x |
| 2 | 1.30719e+09 | 1.30719e+09 | 1.31148e+09 | 1.00x | 1.00x |
| 4 | 2.59740e+09 | 2.60586e+09 | 2.55591e+09 | 1.00x | 0.98x |
| 6 | 3.85852e+09 | 3.89610e+09 | 3.84615e+09 | 1.01x | 1.00x |
| 8 | 5.07937e+09 | 5.09554e+09 | 2.60163e+09 | 1.00x | 0.51x |
| 12 | 6.85714e+09 | 7.31707e+09 | 3.83387e+09 | 1.07x | 0.56x |

### perf Run

| Threads | Unpinned | Pinned Logical | Pinned Spaced | Logical / Unpinned | Spaced / Unpinned |
|---|---:|---:|---:|---:|---:|
| 1 | 6.51466e+08 | 6.51466e+08 | 6.51466e+08 | 1.00x | 1.00x |
| 2 | 1.28617e+09 | 1.29870e+09 | 1.29450e+09 | 1.01x | 1.01x |
| 4 | 2.54777e+09 | 2.58065e+09 | 2.54777e+09 | 1.01x | 1.00x |
| 6 | 3.79747e+09 | 3.82166e+09 | 3.80952e+09 | 1.01x | 1.00x |
| 8 | 5.00000e+09 | 5.00000e+09 | 2.54372e+09 | 1.00x | 0.51x |
| 12 | 6.72269e+09 | 7.10059e+09 | 3.77953e+09 | 1.06x | 0.56x |

## perf Summary

    31387.28 msec task-clock
    4.9 CPUs utilized
    233 page-faults
    141,144,885,621 cpu-cycles
    178,132,908,458 instructions
    1.3 instructions per cycle
    6.343003421 seconds elapsed
    31.174767000 seconds user
    0.023843000 seconds sys

## Interpretation

For 1-6 threads, all placement strategies performed similarly.

This suggests the scheduler already placed independent CPU-bound work reasonably well across available hardware threads.

At 8 and 12 threads, the pinned-spaced configuration collapsed to roughly 50-56% of unpinned throughput.

The reason is that the spaced CPU set only contains 6 CPU IDs:

    0, 2, 4, 6, 8, 10

When running 8 or 12 threads, the benchmark wraps around and pins multiple threads onto the same 6 logical CPUs. This underuses the available 12 logical CPUs and creates artificial contention.

Pinned-logical placement performed similarly to unpinned placement and slightly improved throughput at 12 threads.

## Key Lesson

CPU affinity is not automatically good or bad.

Affinity can improve repeatability and control, but poor placement can reduce performance by:

- underusing available CPUs
- stacking threads onto the same logical CPUs
- fighting the scheduler's default placement
- ignoring SMT/topology behavior

Good affinity strategy requires understanding the CPU topology and the workload.

## Why This Matters

CPU placement matters in systems-performance work.

This experiment introduces:

- Linux CPU affinity
- pthread affinity APIs
- logical CPU placement
- scheduler behavior
- SMT-aware scaling considerations

It builds on earlier Phase 2 experiments by moving from pure thread count scaling into explicit CPU placement control.

