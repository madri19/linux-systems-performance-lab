# Phase 2A Summary - Linux Systems Performance Lab

Phase 2A focused on multicore Linux systems-performance behavior on a desktop-class x86_64 machine.

This phase builds on the ARM Embedded Linux Performance Lab by moving from constrained embedded hardware into modern multicore performance analysis.

## Hardware

- AMD Ryzen 5 5600X
- 6 physical cores
- 12 logical CPUs / SMT threads
- 32MB L3 cache
- x86_64
- Ubuntu on WSL2
- Linux 6.6.114.1-microsoft-standard-WSL2

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

## Experiments Completed

| Experiment | Focus |
|---|---|
| 01_multithread_scaling | Baseline CPU-bound scaling across physical cores and SMT threads |
| 02_false_sharing | Cache-line false sharing and padding/alignment impact |
| 03_mutex_contention | Throughput collapse from shared mutex contention |
| 04_atomic_counter_contention | Mutex vs atomic vs local counter strategies |
| 05_cpu_affinity_scaling | Default scheduler placement vs manual CPU affinity |
| 06_memory_bandwidth_cache | Memory bandwidth and cache locality across working-set sizes |
| 07_allocator_behavior | Heap allocation vs preallocation vs object reuse |
| 08_thread_pool_vs_spawn | Thread spawning vs thread pool reuse across batch workloads |
| 09_producer_consumer_queue | Producer/consumer queue scaling and shared queue contention |
| 10_spsc_ring_buffer | SPSC ring buffer vs mutex-protected queue |
| 11_perf_flamegraph | perf and flamegraph profiling of mutex contention |

## Key Findings

### Independent CPU Work Scales Well

The multithread scaling baseline showed near-linear scaling through the physical core count.

At 6 threads, throughput reached approximately 5.9x the single-thread baseline.

This established that independent CPU-bound work can scale well when threads avoid shared hot state.

### False Sharing Can Destroy Performance

The false sharing benchmark showed more than 10x improvement from separating hot atomic counters onto different cache lines.

This demonstrated that logically independent variables can still interfere if they occupy the same cache line.

### Mutex Contention Can Make More Threads Slower

The mutex contention benchmark showed that adding threads reduced total throughput.

At 12 threads, throughput dropped to roughly 20-25% of the single-thread baseline.

The bottleneck was not CPU availability. The bottleneck was serialization through one shared mutex.

### Atomics Are Not Magic

The atomic counter benchmark showed that a shared atomic counter outperformed a mutex-protected counter, but still scaled poorly.

The per-thread local counter strategy was dramatically faster.

Observed hierarchy:

    mutex contention < shared atomic contention << local aggregation

The main lesson was that avoiding shared hot state is often better than making shared state cheaper.

### CPU Affinity Requires Topology Awareness

The CPU affinity experiment showed that manual pinning can help or hurt.

Pinned logical CPU placement performed similarly to scheduler placement.

A smaller spaced CPU set performed well up to 6 threads, but collapsed at 8 and 12 threads because multiple threads were stacked onto the same limited CPU IDs.

Affinity is useful only when placement matches the hardware topology and workload.

### Memory Locality Dominates Throughput

The memory bandwidth/cache experiment showed that random access became dramatically worse as working-set size increased.

Random read throughput retained:

- about 59% of sequential throughput at 32 KiB
- about 51% at 256 KiB
- about 30-34% at 4 MiB
- about 8-9% at 32 MiB
- about 6-7% at 128 MiB

Sequential access benefited from cache locality and hardware prefetching.

Random access exposed memory latency and cache miss costs.

### Allocation Strategy Matters

The allocator benchmark showed that preallocation and object reuse massively outperformed per-object heap allocation.

Observed speedups vs `new/delete`:

- object pool reuse: about 21-23x faster
- preallocated vector: about 31-34x faster

The most important optimization was avoiding allocation in the hot path.

### Thread Pools Depend on Workload Shape

The thread pool experiment showed that thread pools are not automatically faster.

For one large batch, spawning workers once and using a thread pool performed almost identically.

For many small batches, reusing a thread pool was about 2-3x faster because it avoided repeated thread creation and destruction.

### Shared Queues Become Bottlenecks

The producer/consumer queue experiment showed that adding more producers and consumers reduced throughput when all threads contended on one shared mutex-protected queue.

This demonstrated that queue design can become the scalability limit.

### Simpler Ownership Enables Faster Queues

The SPSC ring buffer benchmark showed that a single-producer/single-consumer ring buffer outperformed a mutex queue.

The SPSC design was faster because the ownership model was simpler:

- one producer
- one consumer
- fixed-size buffer
- no general-purpose mutex
- atomic head/tail coordination

### Profiling Explains Timing Results

The perf/flamegraph experiment connected benchmark timing to hot-path evidence.

The mutex contention flamegraph showed time concentrated in lock and futex-related paths:

- `___pthread_mutex_lock`
- `___pthread_mutex_unlock`
- `__pthread_mutex_unlock_usercnt`
- `lll_mutex_unlock_optimized`
- `__lll_lock_wait`
- `__lll_lock_wake`
- `futex_wait`

This confirmed that the mutex benchmark was dominated by synchronization overhead rather than useful counter increments.

## Main Lessons

Phase 2A demonstrated that multicore performance is usually limited by coordination costs, not just raw CPU count.

The recurring bottlenecks were:

- shared hot state
- cache-line ownership
- lock contention
- poor locality
- queue contention
- allocation in hot paths
- bad CPU placement
- unnecessary synchronization

The recurring fixes were:

- avoid shared writes
- use per-thread state
- batch work
- reduce synchronization frequency
- improve memory locality
- preallocate memory
- use ownership-aware queues
- profile before guessing

## Relationship to Phase 1

Phase 1 focused on embedded ARM Linux fundamentals:

- Buildroot
- ARM userspace behavior
- networking
- syscall behavior
- scheduler timing
- constrained hardware observation

Phase 2A moved into modern multicore systems behavior:

- thread scaling
- cache effects
- synchronization bottlenecks
- memory hierarchy
- allocator behavior
- profiling workflows

Together, the two phases connect low-level Linux systems work with multicore performance analysis.

## Next Step - Phase 2B

Phase 2B should rerun selected Phase 2A benchmarks on ARM server hardware.

Target platforms:

- AWS Graviton
- Ampere Altra
- other ARM Neoverse-class instances

Useful reruns:

- multithread scaling
- false sharing
- mutex contention
- atomic counter contention
- CPU affinity/topology
- memory bandwidth/cache locality
- allocator behavior
- SPSC ring buffer
- perf/flamegraph analysis

The goal is to compare x86_64 Ryzen behavior against server-class ARM behavior using the same benchmark suite.

That creates a bridge between:

    desktop Linux systems performance
    and
    ARM server-class performance analysis

## Status

Phase 2A is complete.

