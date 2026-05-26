# Experiment 08 - Thread Pool vs Thread Spawning

Compares spawning worker threads against reusing a fixed thread pool.

## Goal

Measure when a thread pool helps and when it does not.

This experiment compares two scenarios:

- one large batch of tasks
- many small batches of tasks

The goal is to show that thread pools are useful when they amortize repeated thread creation and destruction, but they are not automatically faster for every workload.

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

Each task runs a small CPU-bound integer workload.

Worker threads:

    6

Work per task:

    10000 iterations

Two scenarios are measured.

## Scenario A - One Large Batch

The benchmark processes:

    10000 tasks

Two approaches are compared:

- spawn 6 worker threads once
- submit 10000 tasks to a thread pool

## Scenario B - Many Small Batches

The benchmark processes:

    1000 batches
    24 tasks per batch
    24000 total tasks

Two approaches are compared:

- spawn 6 worker threads for every batch
- reuse one persistent thread pool across all batches

## Results

### Native Run

| Scenario | Approach | Elapsed | Throughput |
|---|---|---:|---:|
| One large batch | Spawn workers once | 26535 us | 376861 tasks/sec |
| One large batch | Thread pool | 26531 us | 376918 tasks/sec |
| Many small batches | Spawn workers every batch | 314196 us | 76385.4 tasks/sec |
| Many small batches | Thread pool reused | 150393 us | 159582 tasks/sec |

Ratios:

    Large batch thread pool / spawn: 1.00015x
    Small batches thread pool / spawn: 2.08917x

### perf Run

| Scenario | Approach | Elapsed | Throughput |
|---|---|---:|---:|
| One large batch | Spawn workers once | 27069 us | 369426 tasks/sec |
| One large batch | Thread pool | 26947 us | 371099 tasks/sec |
| Many small batches | Spawn workers every batch | 468129 us | 51267.9 tasks/sec |
| Many small batches | Thread pool reused | 170798 us | 140517 tasks/sec |

Ratios:

    Large batch thread pool / spawn: 1.00453x
    Small batches thread pool / spawn: 2.74083x

## perf Summary

    2000.50 msec task-clock
    2.8 CPUs utilized
    4269 page-faults
    5,073,525,708 cpu-cycles
    6,296,977,354 instructions
    1.2 instructions per cycle
    0.696293700 seconds elapsed
    0.810551000 seconds user
    0.758630000 seconds sys

## Interpretation

For one large batch, spawning 6 workers once performed almost identically to using a thread pool.

This happens because thread creation cost is paid only once and is amortized across 10000 tasks.

For many small batches, the thread pool was much faster.

In the native run, the thread pool was about 2.09x faster.

In the perf run, the thread pool was about 2.74x faster.

This happens because the spawn-per-batch approach repeatedly creates and destroys worker threads, while the thread pool keeps workers alive and reuses them.

## Key Lesson

Thread pools are not automatically faster.

They help when worker reuse avoids repeated thread creation and destruction.

They may not help much when:

- the workload is one large batch
- thread creation is already amortized
- per-task queue synchronization overhead dominates

They help more when:

- tasks arrive in repeated batches
- work is bursty
- worker startup costs would otherwise repeat
- the system benefits from persistent workers

## Why This Matters

Thread pools are common in systems software, services, game engines, runtimes, and performance-sensitive infrastructure.

This experiment shows that the benefit comes from matching the execution model to the workload.

The important question is not simply:

    Should I use a thread pool?

The better question is:

    What overhead am I trying to amortize?

