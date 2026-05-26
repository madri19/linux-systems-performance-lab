# Experiment 02 - False Sharing

Measures the performance impact of false sharing between two threads on a multicore Ryzen system.

## Goal

Show how memory layout can destroy multicore scalability even when threads update logically independent variables.

This experiment compares:

- two atomic counters placed next to each other in memory
- two atomic counters separated onto different cache lines using padding

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

Two threads each increment their own atomic counter.

Each thread performs:

    200,000,000 atomic increments

Two layouts are measured.

### False Sharing Layout

Both counters are adjacent in memory.

Although each thread updates a different variable, both variables likely occupy the same cache line.

This causes the cache line to bounce between cores.

### Padded Layout

Each counter is aligned to its own 64-byte cache line.

This prevents the two threads from fighting over ownership of the same cache line.

## Results

### Native Run

| Layout | Time |
|---|---:|
| False sharing | 3405 ms |
| Padded | 296 ms |

Speedup from padding:

    11.5034x

### perf Run

| Layout | Time |
|---|---:|
| False sharing | 3491 ms |
| Padded | 329 ms |

Speedup from padding:

    10.6109x

## perf Summary

    7629.44 msec task-clock
    2.0 CPUs utilized
    144 page-faults
    34,728,655,410 cpu-cycles
    3,236,591,217 instructions
    0.1 instructions per cycle
    3.822572893 seconds elapsed
    7.622692000 seconds user
    0.000000000 seconds sys

## Interpretation

The padded layout was more than 10x faster than the false-sharing layout.

Both versions used:

- two threads
- two atomic counters
- the same number of increments
- the same logical workload

The major difference was memory layout.

In the false-sharing case, independent counters likely shared one cache line. Each atomic update required exclusive ownership of that cache line, causing cache-coherence traffic between cores.

In the padded case, each counter had its own cache line, so each thread could update independently with much less cache-line contention.

The low IPC in the perf run reflects how much time the CPU spent stalled around memory/cache-coherence behavior instead of retiring useful instructions.

## Why This Matters

False sharing is one of the most important multicore performance pitfalls.

This experiment shows that:

- correctness does not imply scalability
- independent variables can still interfere
- memory layout can dominate performance
- cache-line ownership matters
- padding/alignment can produce major speedups

This builds directly on Experiment 01 by showing how a workload can fail to scale when shared memory behavior is introduced.

