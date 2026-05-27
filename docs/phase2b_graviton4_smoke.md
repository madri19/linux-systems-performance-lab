# Phase 2B - Graviton4 Smoke Run

This document records the first ARM server smoke run of the Phase 2 benchmark suite.

## Target

- AWS C8g
- Instance size: c8g.xlarge
- Architecture: aarch64
- CPU core: Arm Neoverse-V2
- vCPUs: 4
- OS: Ubuntu 24.04
- Kernel: AWS 6.17

## Purpose

The goal was to verify that the Phase 2A Linux systems-performance benchmarks build and run on server-class ARM hardware.

This is not yet the final x86-vs-ARM comparison because c8g.xlarge has only 4 vCPUs.

Thread counts above 4 represent oversubscription.

## Result Files

See:

- `results/graviton4_c8g/environment.txt`
- `results/graviton4_c8g/01_multithread_scaling.txt`
- `results/graviton4_c8g/02_false_sharing.txt`
- `results/graviton4_c8g/03_mutex_contention.txt`
- `results/graviton4_c8g/04_atomic_counter_contention.txt`
- `results/graviton4_c8g/06_memory_bandwidth_cache.txt`
- `results/graviton4_c8g/06_memory_bandwidth_cache_perf.txt`
- `results/graviton4_c8g/07_allocator_behavior.txt`
- `results/graviton4_c8g/10_spsc_ring_buffer.txt`

## Initial Observations

Experiment 01 showed ideal scaling through 4 vCPUs and saturation beyond that point.

Experiment 02 showed false-sharing padding improvement of about 3.31x.

Experiment 03 showed severe mutex contention collapse.

Experiment 04 showed atomics dramatically outperforming mutexes under contention, while local counters remained much faster than both.

Experiment 06 showed random memory access degrading as working-set size increased. The perf run showed low IPC, large cache refill counts, and high DTLB walk counts.

Experiment 07 showed preallocation and object reuse outperforming per-object heap allocation.

Experiment 10 showed the SPSC ring buffer outperforming the mutex queue.

## perf Notes

Default `perf stat` failed on topdown slot metrics.

Explicit ARM PMU events were used instead.

`perf_event_paranoid` was temporarily changed with:

    sudo sysctl kernel.perf_event_paranoid=-1

L3 events reported zero in this VM/kernel/PMU setup, so L3 counters from this run are not used for conclusions.

## Next Step

For a cleaner comparison against the Ryzen 5600X Phase 2A machine, rerun on a larger Graviton4 instance with at least 12 vCPUs, such as c8g.4xlarge or larger.
