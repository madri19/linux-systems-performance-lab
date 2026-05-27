# Graviton4 C8g Results

Phase 2B smoke run on AWS Graviton4.

## Instance

- Instance family: C8g
- Size: c8g.xlarge
- Architecture: aarch64
- Core: Arm Neoverse-V2
- vCPUs: 4
- Threads per core: 1
- L1d: 256 KiB total across 4 cores
- L1i: 256 KiB total across 4 cores
- L2: 8 MiB total across 4 cores
- L3: 36 MiB shared
- OS: Ubuntu 24.04 on AWS kernel 6.17

## Caveats

This instance has 4 vCPUs.

Benchmarks that run 6, 8, or 12 threads oversubscribe the instance. Those results show saturation behavior, not true 6/8/12-core scaling.

This is a smoke test proving the Phase 2A benchmark suite builds and runs on Neoverse-V2 ARM server hardware.

For a cleaner comparison against the Ryzen 5600X Phase 2A machine, rerun on a larger instance such as c8g.4xlarge or larger.

## perf Notes

Default `perf stat` attempted topdown slot metrics and failed with:

    Failure to read '#slots'

Explicit ARM PMU events were used instead.

Initial `perf_event_paranoid` was 4, which blocked counter access. Temporary fix:

    sudo sysctl kernel.perf_event_paranoid=-1

L3 events reported zero in the current VM/kernel/PMU setup, so L3 counters should not be used for conclusions from this run.

## Initial Findings

Experiment 01 showed perfect scaling through 4 vCPUs, then saturation from oversubscription.

Experiment 02 showed false-sharing padding improvement of about 3.31x.

Experiment 03 showed severe mutex contention collapse.

Experiment 04 showed shared atomics dramatically outperforming mutex contention, while local counters were much faster than both.

Experiment 06 showed random memory access degrading as working set size increased. The perf run showed low IPC and high DTLB/cache refill activity, supporting the memory-locality bottleneck interpretation.

Experiment 07 showed preallocation and object reuse outperforming per-object heap allocation.

Experiment 10 showed SPSC ring buffer throughput beating a mutex queue.

