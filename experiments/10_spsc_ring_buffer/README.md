# Experiment 10 - SPSC Ring Buffer vs Mutex Queue

Compares a mutex-protected queue against a single-producer/single-consumer ring buffer.

## Goal

Show how a simpler ownership model can reduce synchronization overhead.

This experiment compares:

- mutex-protected blocking queue
- SPSC ring buffer using atomic head/tail indices

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

The benchmark transfers:

    5,000,000 items

between:

- one producer
- one consumer

Two queue designs are measured.

### Mutex Queue

Uses:

- `std::queue`
- `std::mutex`
- `std::condition_variable`

### SPSC Ring Buffer

Uses:

- fixed-size circular buffer
- atomic head index
- atomic tail index
- cache-line-aligned indices
- single producer ownership of head
- single consumer ownership of tail

Ring capacity:

    65536 entries

## Results

### Native Run

| Queue | Elapsed | Throughput |
|---|---:|---:|
| Mutex queue | 342152 us | 1.46134e+07 items/sec |
| SPSC ring buffer | 93320 us | 5.35791e+07 items/sec |

Speedup:

    SPSC ring / mutex queue: 3.66644x

### perf Run

| Queue | Elapsed | Throughput |
|---|---:|---:|
| Mutex queue | 160731 us | 3.11079e+07 items/sec |
| SPSC ring buffer | 94321 us | 5.30105e+07 items/sec |

Speedup:

    SPSC ring / mutex queue: 1.70408x

## perf Summary

    450.54 msec task-clock
    1.7 CPUs utilized
    9443 page-faults
    1,246,045,943 cpu-cycles
    1,135,683,437 instructions
    0.9 instructions per cycle
    0.257626148 seconds elapsed
    0.302810000 seconds user
    0.117086000 seconds sys

## Interpretation

The SPSC ring buffer was faster than the mutex queue.

In the native run, the ring buffer achieved approximately 3.67x higher throughput.

In the perf run, the ring buffer achieved approximately 1.70x higher throughput.

The SPSC design wins because the problem is constrained:

- one producer writes
- one consumer reads
- no multiple-producer contention
- no multiple-consumer contention

This allows the queue to avoid a general-purpose mutex and condition variable.

The ring buffer uses atomic head/tail indices instead of locking the entire queue.

## Key Lesson

Simpler ownership models enable simpler synchronization.

A general blocking queue must support arbitrary producer/consumer contention.

An SPSC queue only needs to coordinate one producer with one consumer.

That reduced synchronization model can produce much higher throughput.

## Why This Matters

High-performance systems often avoid general-purpose synchronization when ownership is known.

Common strategies include:

- SPSC queues
- per-thread queues
- sharding
- ownership partitioning
- batching
- fixed-size ring buffers
- reduced shared state

This experiment builds directly on Experiment 09, where a shared mutex queue became a scalability bottleneck.

