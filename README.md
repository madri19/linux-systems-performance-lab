# Linux Systems Performance Lab

Phase 2 systems-performance experiments on a multicore Linux workstation.

This repository focuses on modern Linux systems behavior on desktop-class x86_64 hardware.

## Phase 2 Scope

Phase 2 builds on the ARM Embedded Linux Performance Lab and moves from constrained embedded hardware into multicore Linux performance analysis.

Focus areas:

- multicore scaling
- thread contention
- lock-free structures
- cache locality
- false sharing
- scheduler behavior
- CPU affinity
- perf/flamegraphs
- memory allocator behavior
- workload benchmarking

## Hardware Platform

- AMD Ryzen 5 5600X
- 6 physical cores
- 12 logical CPUs / SMT threads
- 32MB L3 cache
- x86_64 Linux via WSL2

## Tooling

- GCC
- Clang
- CMake
- Ninja
- perf
- strace
- htop
- gdb

## Experiments

| Experiment | Description |
|---|---|
| [01_multithread_scaling](experiments/01_multithread_scaling) | Measuring CPU-bound throughput scaling across physical cores and SMT threads |
| [02_false_sharing](experiments/02_false_sharing) | Measuring cache-line false sharing and the impact of padding/alignment |
| [03_mutex_contention](experiments/03_mutex_contention) | Measuring throughput collapse from mutex contention across multiple threads |
| [04_atomic_counter_contention](experiments/04_atomic_counter_contention) | Comparing mutex, shared atomic, and per-thread local counter strategies |
| [05_cpu_affinity_scaling](experiments/05_cpu_affinity_scaling) | Comparing default scheduler placement with manual CPU affinity strategies |
| [06_memory_bandwidth_cache](experiments/06_memory_bandwidth_cache) | Measuring memory bandwidth and cache locality effects across working-set sizes |
| [07_allocator_behavior](experiments/07_allocator_behavior) | Comparing heap allocation, preallocation, and object pool reuse strategies |
| [08_thread_pool_vs_spawn](experiments/08_thread_pool_vs_spawn) | Comparing thread spawning with thread pool reuse across batch workloads |
| [09_producer_consumer_queue](experiments/09_producer_consumer_queue) | Measuring producer/consumer queue scaling and shared queue contention |
| [10_spsc_ring_buffer](experiments/10_spsc_ring_buffer) | Comparing SPSC ring buffer throughput against a mutex-protected queue |
| [11_perf_flamegraph](experiments/11_perf_flamegraph) | Profiling mutex contention with perf and flamegraph analysis |
| [07_allocator_behavior](experiments/07_allocator_behavior) | Comparing heap allocation, preallocation, and object pool reuse strategies |
| [08_thread_pool_vs_spawn](experiments/08_thread_pool_vs_spawn) | Comparing thread spawning with thread pool reuse across batch workloads |
| [09_producer_consumer_queue](experiments/09_producer_consumer_queue) | Measuring producer/consumer queue scaling and shared queue contention |
| [10_spsc_ring_buffer](experiments/10_spsc_ring_buffer) | Comparing SPSC ring buffer throughput against a mutex-protected queue |
| [11_perf_flamegraph](experiments/11_perf_flamegraph) | Profiling mutex contention with perf and flamegraph analysis |

## Related Project

- https://github.com/madri19/arm-embedded-linux-performance-lab

