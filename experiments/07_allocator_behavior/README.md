# Experiment 07 - Allocator Behavior

Compares several object allocation strategies on a Ryzen 5 5600X system.

## Goal

Measure the performance impact of allocation strategy.

This experiment compares:

- `new/delete`
- `malloc/free`
- preallocated `std::vector`
- object pool reuse

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

Each strategy creates or reuses:

    1,000,000 objects per iteration

The benchmark runs:

    10 iterations

Each object is a small `Node` containing:

    int value;
    Node* next;

Throughput is measured in objects per second.

## Results

### Native Run

| Strategy | Elapsed | Throughput |
|---|---:|---:|
| new/delete | 215999 us | 4.62965e+07 objects/sec |
| malloc/free | 174072 us | 5.74475e+07 objects/sec |
| vector preallocated | 6878 us | 1.45391e+09 objects/sec |
| object pool reuse | 10164 us | 9.83865e+08 objects/sec |

Speedups vs `new/delete`:

    object pool / new-delete: 21.2514x
    vector / new-delete:      31.4043x

### perf Run

| Strategy | Elapsed | Throughput |
|---|---:|---:|
| new/delete | 227013 us | 4.40503e+07 objects/sec |
| malloc/free | 176383 us | 5.66948e+07 objects/sec |
| vector preallocated | 6635 us | 1.50716e+09 objects/sec |
| object pool reuse | 10033 us | 9.96711e+08 objects/sec |

Speedups vs `new/delete`:

    object pool / new-delete: 22.6266x
    vector / new-delete:      34.2145x

## perf Summary

    421.74 msec task-clock
    1.0 CPUs utilized
    11,859 page-faults
    1,771,366,841 cpu-cycles
    7,963,773,110 instructions
    4.5 instructions per cycle
    0.424693562 seconds elapsed
    0.383604000 seconds user
    0.039958000 seconds sys

## Interpretation

Per-object heap allocation was much slower than preallocation and reuse.

`malloc/free` was faster than `new/delete` in this benchmark, but both were far slower than avoiding per-object allocation entirely.

The fastest strategy was preallocated contiguous `std::vector` storage.

The object pool was also dramatically faster than heap allocation because it reused already-allocated memory and avoided allocator calls in the hot path.

The main performance differences come from:

- allocator overhead
- metadata bookkeeping
- pointer chasing
- cache locality
- contiguous storage
- avoiding hot-path allocation

## Key Lesson

Allocation strategy can dominate runtime performance.

The hierarchy observed here was:

    new/delete < malloc/free << object pool reuse < vector preallocated

The biggest optimization was not making allocation faster.

It was avoiding repeated allocation entirely.

## Why This Matters

Real systems often suffer from allocation overhead in hot paths.

This experiment shows why performance-sensitive code often uses:

- preallocation
- object pools
- arenas
- slab allocators
- reusable buffers
- contiguous data layouts

This builds on earlier memory locality experiments by showing how allocation strategy affects throughput and cache behavior.

