# Experiment 09 - Producer/Consumer Queue Scaling

Measures how a mutex-protected blocking queue scales as producer and consumer thread counts increase.

## Goal

Show that adding more producers and consumers does not automatically improve throughput when all threads contend on one shared queue.

This experiment compares:

- 1 producer / 1 consumer
- 2 producers / 2 consumers
- 4 producers / 4 consumers
- 6 producers / 6 consumers

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

Each producer generates:

    500,000 items

Items are pushed into one shared blocking queue.

Consumers pop items from the same queue until all producers finish and the queue is drained.

The queue uses:

- `std::queue`
- `std::mutex`
- `std::condition_variable`

Throughput is calculated as:

    total consumed items / elapsed time

## Results

### Native Run

| Producers | Consumers | Items | Elapsed | Throughput | Vs 1P/1C |
|---:|---:|---:|---:|---:|---:|
| 1 | 1 | 500,000 | 23100 us | 2.16450e+07 items/sec | 1.00x |
| 2 | 2 | 1,000,000 | 69590 us | 1.43699e+07 items/sec | 0.66x |
| 4 | 4 | 2,000,000 | 175399 us | 1.14026e+07 items/sec | 0.53x |
| 6 | 6 | 3,000,000 | 376659 us | 7.96476e+06 items/sec | 0.37x |

### perf Run

| Producers | Consumers | Items | Elapsed | Throughput | Vs 1P/1C |
|---:|---:|---:|---:|---:|---:|
| 1 | 1 | 500,000 | 16295 us | 3.06843e+07 items/sec | 1.00x |
| 2 | 2 | 1,000,000 | 40675 us | 2.45851e+07 items/sec | 0.80x |
| 4 | 4 | 2,000,000 | 99083 us | 2.01851e+07 items/sec | 0.66x |
| 6 | 6 | 3,000,000 | 233712 us | 1.28363e+07 items/sec | 0.42x |

## perf Summary

    3182.46 msec task-clock
    7.8 CPUs utilized
    1764 page-faults
    1,560,343,229 cpu-cycles
    1,445,575,688 instructions
    0.9 instructions per cycle
    0.392152491 seconds elapsed
    0.385278000 seconds user
    2.161614000 seconds sys

## Interpretation

Throughput decreased as more producers and consumers were added.

In the native run:

    1P/1C: 21.6M items/sec
    6P/6C: 8.0M items/sec

In the perf run:

    1P/1C: 30.7M items/sec
    6P/6C: 12.8M items/sec

The shared queue became a synchronization bottleneck.

Every producer and consumer contends on the same queue mutex. As thread count increases, useful work does not scale because access to the queue is serialized.

The perf result shows heavy system overhead:

    user time: 0.385s
    system time: 2.162s

Most runtime was spent in system/kernel overhead rather than useful application work.

## Key Lesson

A shared queue can become the scalability limit.

Adding more producer and consumer threads can reduce total throughput when they all contend on the same synchronization point.

This experiment shows that scalable producer/consumer systems often need:

- queue sharding
- batching
- per-thread queues
- lock-free or wait-free structures
- reduced synchronization frequency
- careful ownership design

## Why This Matters

Producer/consumer queues are common in systems software, runtimes, services, thread pools, logging systems, networking stacks, and job schedulers.

This experiment shows that the queue itself can become the bottleneck.

It builds on earlier Phase 2 experiments:

- mutex contention
- atomic contention
- thread pool task queues
- CPU scaling

