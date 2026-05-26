# Experiment 06 - Memory Bandwidth and Cache Locality

Measures how memory access pattern and working-set size affect throughput on a Ryzen 5 5600X system.

## Goal

Investigate how cache locality impacts memory throughput.

This experiment compares:

- sequential read
- sequential write
- random read
- random write

across multiple working-set sizes.

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

The benchmark allocates arrays of increasing size:

- 32 KiB
- 256 KiB
- 4 MiB
- 32 MiB
- 128 MiB

For each working set, the benchmark measures:

- sequential read throughput
- sequential write throughput
- random read throughput
- random write throughput

Each working set is repeated 20 times.

Throughput is reported in MiB/s.

## Results

### Native Run

| Working Set | Sequential Read | Sequential Write | Random Read | Random Write | Random Read / Sequential Read |
|---|---:|---:|---:|---:|---:|
| 32 KiB | 32894.7 MiB/s | 36764.7 MiB/s | 19531.2 MiB/s | 27173.9 MiB/s | 0.59x |
| 256 KiB | 31055.9 MiB/s | 35461.0 MiB/s | 15772.9 MiB/s | 13297.9 MiB/s | 0.51x |
| 4096 KiB | 30639.6 MiB/s | 35057.0 MiB/s | 9086.78 MiB/s | 11095.7 MiB/s | 0.30x |
| 32768 KiB | 19940.8 MiB/s | 23722.2 MiB/s | 1693.19 MiB/s | 2277.94 MiB/s | 0.08x |
| 131072 KiB | 18036.3 MiB/s | 13315.0 MiB/s | 1173.6 MiB/s | 989.109 MiB/s | 0.07x |

### perf Run

| Working Set | Sequential Read | Sequential Write | Random Read | Random Write | Random Read / Sequential Read |
|---|---:|---:|---:|---:|---:|
| 32 KiB | 32894.7 MiB/s | 36764.7 MiB/s | 19531.2 MiB/s | 27173.9 MiB/s | 0.59x |
| 256 KiB | 30864.2 MiB/s | 35461.0 MiB/s | 16077.2 MiB/s | 13157.9 MiB/s | 0.52x |
| 4096 KiB | 29784.1 MiB/s | 34320.0 MiB/s | 9997.5 MiB/s | 10905.1 MiB/s | 0.34x |
| 32768 KiB | 20062.1 MiB/s | 22453.8 MiB/s | 1836.22 MiB/s | 2254.98 MiB/s | 0.09x |
| 131072 KiB | 17901.3 MiB/s | 13497.5 MiB/s | 1239.72 MiB/s | 997.134 MiB/s | 0.07x |

## perf Summary

    6406.55 msec task-clock
    1.0 CPUs utilized
    84,258 page-faults
    28,073,279,639 cpu-cycles
    12,271,830,863 instructions
    0.4 instructions per cycle
    6.407573556 seconds elapsed
    6.175200000 seconds user
    0.231969000 seconds sys

## Interpretation

Sequential access remained much faster than random access across all working-set sizes.

As the working set increased, random access performance collapsed sharply:

- 32 KiB random read retained about 59% of sequential read throughput
- 256 KiB retained about 51%
- 4 MiB retained about 30-34%
- 32 MiB retained about 8-9%
- 128 MiB retained about 6-7%

Sequential access benefits from:

- spatial locality
- cache-line utilization
- hardware prefetching
- predictable memory access patterns

Random access loses those advantages as the working set grows.

The 32 MiB and 128 MiB cases show especially large drops because the workload no longer fits comfortably in faster cache levels, causing more memory-latency exposure.

The low IPC in the perf run is consistent with a memory-bound workload.

## Key Lesson

Memory layout and access pattern can dominate performance.

The same amount of data can run dramatically faster or slower depending on whether access is sequential and cache-friendly or random and cache-unfriendly.

## Why This Matters

This experiment introduces cache hierarchy and memory bandwidth behavior into the Phase 2 lab.

It builds on earlier experiments by moving from synchronization bottlenecks into memory-system bottlenecks.

This matters because many real systems-performance problems are caused by:

- poor locality
- cache misses
- memory bandwidth limits
- unpredictable access patterns
- inefficient data layout

