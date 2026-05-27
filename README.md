# Linux Systems Performance Lab

Phase 2 systems-performance lab focused on multicore Linux behavior, first on x86_64 desktop hardware and then on ARM server-class hardware.

This project builds on the related ARM Embedded Linux Performance Lab and moves from constrained embedded Linux into modern multicore performance analysis.

## Project Structure

Phase 2 is split into two parts:

| Phase | Platform | Purpose |
|---|---|---|
| Phase 2A | Ryzen 5 5600X / x86_64 / WSL2 | Build the benchmark suite and study multicore Linux performance behavior |
| Phase 2B | AWS Graviton4 / Neoverse-V2 / aarch64 | Validate selected benchmarks on ARM server-class hardware |

## Phase 2A - x86_64 Multicore Benchmark Suite

Phase 2A is the main benchmark-development phase.

It runs on a desktop-class Linux environment:

- AMD Ryzen 5 5600X
- 6 physical cores
- 12 logical CPUs / SMT threads
- 32MB L3 cache
- x86_64 Linux via WSL2

Phase 2A studies:

- multicore scaling
- thread contention
- cache locality
- false sharing
- mutex contention
- atomic contention
- scheduler behavior
- CPU affinity
- memory bandwidth
- allocator behavior
- thread pools
- producer/consumer queues
- SPSC ring buffers
- perf/flamegraph profiling

## Phase 2B - ARM Server Smoke Run

Phase 2B reruns selected Phase 2A benchmarks on AWS Graviton4.

The first smoke run used:

- AWS C8g
- c8g.xlarge
- Arm Neoverse-V2
- aarch64
- 4 vCPUs
- 36 MiB shared L3
- Ubuntu 24.04

Results are stored under:

- [results/graviton4_c8g](results/graviton4_c8g)

The goal of Phase 2B is to prove the benchmark suite runs on ARM server-class hardware and begin comparing x86_64 desktop behavior against Neoverse-V2 ARM behavior.

The c8g.xlarge run is a smoke test, not a full 12-thread comparison. Since the instance has 4 vCPUs, benchmark runs above 4 threads represent oversubscription.

## Tooling

- GCC
- Clang
- CMake
- Ninja
- perf
- FlameGraph
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

## Results and Summaries

| Document | Description |
|---|---|
| [docs/phase2_summary.md](docs/phase2_summary.md) | Final Phase 2 summary covering Phase 2A and Phase 2B |
| [docs/phase2a_summary.md](docs/phase2a_summary.md) | x86_64 Ryzen / WSL2 benchmark-suite summary |
| [docs/phase2b_graviton4_smoke.md](docs/phase2b_graviton4_smoke.md) | Graviton4 / Neoverse-V2 smoke-run summary |
| [results/graviton4_c8g](results/graviton4_c8g) | Raw Graviton4 smoke-run result files |

## Key Lessons

Phase 2 shows that multicore performance is often limited by coordination and locality costs, not raw CPU count.

Recurring bottlenecks:

- shared hot state
- cache-line ownership
- lock contention
- atomic contention
- poor memory locality
- allocator overhead
- queue contention
- oversubscription
- unnecessary synchronization

Recurring fixes:

- per-thread state
- batching
- preallocation
- cache-line padding
- ownership-aware queues
- reduced synchronization frequency
- profiling before guessing

## Related Project

- https://github.com/madri19/arm-embedded-linux-performance-lab
