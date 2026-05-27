# Phase 2 Summary - Linux Systems Performance Lab

Phase 2 expanded the systems-performance work from constrained embedded ARM Linux into multicore Linux performance analysis and ARM server validation.

## Structure

Phase 2 has two parts:

| Phase | Platform | Purpose |
|---|---|---|
| Phase 2A | Ryzen 5 5600X / x86_64 / WSL2 | Build the benchmark suite and study multicore Linux behavior |
| Phase 2B | AWS Graviton4 / Neoverse-V2 / aarch64 | Validate the benchmark suite on ARM server-class hardware |

## Phase 2A - x86_64 Multicore Baseline

Phase 2A ran on:

- AMD Ryzen 5 5600X
- 6 physical cores
- 12 logical CPUs / SMT threads
- 32MB L3 cache
- Ubuntu on WSL2
- x86_64

Phase 2A experiments:

| Experiment | Focus |
|---|---|
| 01_multithread_scaling | CPU-bound scaling across cores and SMT threads |
| 02_false_sharing | Cache-line false sharing and padding |
| 03_mutex_contention | Shared mutex contention |
| 04_atomic_counter_contention | Mutex vs atomic vs local aggregation |
| 05_cpu_affinity_scaling | Scheduler placement and manual CPU affinity |
| 06_memory_bandwidth_cache | Memory bandwidth and cache locality |
| 07_allocator_behavior | Heap allocation, preallocation, object reuse |
| 08_thread_pool_vs_spawn | Worker spawning vs thread-pool reuse |
| 09_producer_consumer_queue | Shared producer/consumer queue contention |
| 10_spsc_ring_buffer | SPSC ring buffer vs mutex queue |
| 11_perf_flamegraph | perf/flamegraph bottleneck analysis |

## Phase 2A Main Findings

Independent CPU-bound work scaled well through physical cores.

False sharing caused major performance loss and padding hot counters improved throughput by over 10x.

Mutex contention made more threads slower because the shared lock serialized execution.

Shared atomics were faster than mutexes but still much slower than per-thread local aggregation.

Manual CPU affinity only helped when placement matched the actual topology.

Sequential memory access strongly outperformed random access as working sets grew.

Preallocation and object reuse dramatically outperformed per-object heap allocation.

Thread pools helped when they amortized repeated thread creation, but were not automatically faster for one large batch.

Shared queues became scalability bottlenecks under producer/consumer contention.

A constrained SPSC ownership model enabled a faster ring-buffer design than a general mutex queue.

perf/flamegraph analysis connected timing results to actual lock/futex hot paths.

## Phase 2B - Graviton4 / Neoverse-V2 Smoke Run

Phase 2B ran a smoke subset on AWS Graviton4:

- AWS C8g
- c8g.xlarge
- aarch64
- Arm Neoverse-V2
- 4 vCPUs
- 36 MiB shared L3
- Ubuntu 24.04
- AWS kernel 6.17

Result directory:

    results/graviton4_c8g/

Phase 2B result files:

- environment.txt
- 01_multithread_scaling.txt
- 02_false_sharing.txt
- 03_mutex_contention.txt
- 04_atomic_counter_contention.txt
- 06_memory_bandwidth_cache.txt
- 06_memory_bandwidth_cache_perf.txt
- 07_allocator_behavior.txt
- 10_spsc_ring_buffer.txt

## Phase 2B Main Findings

The benchmark suite built and ran successfully on ARM server-class hardware.

The Graviton4 instance reported:

    Architecture: aarch64
    Model name: Neoverse-V2
    CPU(s): 4

Multithread scaling showed ideal scaling through 4 vCPUs and saturation beyond that point.

False-sharing padding improved throughput by about 3.31x.

Mutex contention collapsed severely on the ARM server instance.

Shared atomics dramatically outperformed mutex contention, while local counters remained much faster than both.

Memory locality effects reproduced on ARM: random access degraded as working-set size increased.

The perf run for memory bandwidth/cache showed:

    cpu_cycles:        20.48B
    inst_retired:      12.37B
    IPC:               about 0.60
    l1d_cache_refill:  894.7M
    l2d_cache_refill:  537.5M
    dtlb_walk:         764.9M

This supports the interpretation that large random-access workloads are cache/TLB/memory dominated.

Preallocation and object reuse outperformed per-object heap allocation.

The SPSC ring buffer outperformed the mutex queue.

## Phase 2B Caveats

The c8g.xlarge instance has only 4 vCPUs.

Any 6, 8, or 12 thread results oversubscribe the instance. Those measurements are useful for observing saturation behavior, but they are not a clean 12-thread comparison against the Ryzen 5600X system.

This was a smoke run, not the final architecture comparison.

For a cleaner x86-vs-ARM comparison, rerun on a larger Graviton4 instance such as c8g.4xlarge or larger.

Default `perf stat` topdown metrics failed on this VM with:

    Failure to read '#slots'

Explicit ARM PMU events were used instead.

The initial `perf_event_paranoid` value was 4 and blocked counter access until temporarily changed with:

    sudo sysctl kernel.perf_event_paranoid=-1

L3 PMU events reported zero in this VM/kernel/PMU configuration, so L3 counter values from this smoke run are not used for conclusions.

## Overall Phase 2 Lessons

Phase 2 showed that multicore performance is usually dominated by coordination and locality costs.

The major bottlenecks were:

- shared hot state
- cache-line ownership
- lock contention
- atomic contention
- memory locality loss
- TLB/cache refill pressure
- allocator overhead
- queue contention
- oversubscription
- synchronization frequency

The recurring fixes were:

- use per-thread state
- avoid shared writes
- batch work
- reduce synchronization
- preallocate memory
- improve locality
- align/pad hot fields
- use ownership-aware queues
- profile hot paths before guessing

## Relationship to Phase 1

Phase 1 focused on embedded ARM Linux:

- Buildroot
- ARM board workflow
- constrained hardware behavior
- Linux process/syscall/network basics

Phase 2 focused on multicore Linux systems performance:

- scaling
- contention
- cache locality
- synchronization
- memory behavior
- perf analysis
- ARM server validation

Together, Phase 1 and Phase 2 form a progression:

    embedded ARM Linux fundamentals
    -> multicore x86 systems performance
    -> ARM server-class performance validation

## Status

Phase 2 is complete.

