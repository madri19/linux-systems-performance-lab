# Experiment 11 - perf Flamegraph Analysis

Profiles the mutex contention benchmark using `perf` and generates a flamegraph.

## Goal

Move beyond benchmark timing and inspect where CPU time is actually spent.

This experiment profiles Experiment 03 using:

- `perf record`
- `perf report`
- `perf script`
- Brendan Gregg's FlameGraph tools

## Target Benchmark

Profiled benchmark:

    experiments/03_mutex_contention/mutex_contention.cpp

Profiled binary:

    build/mutex_contention_profile

Compilation flags:

    -O2 -g -fno-omit-frame-pointer -std=c++20 -pthread

## Commands Used

Build profiling binary:

    g++ -O2 -g -fno-omit-frame-pointer -std=c++20 -pthread \
      experiments/03_mutex_contention/mutex_contention.cpp \
      -o build/mutex_contention_profile

Record profile:

    perf record -F 99 -g \
      -o experiments/11_perf_flamegraph/results/perf.data \
      ./build/mutex_contention_profile

Generate text report:

    perf report \
      -i experiments/11_perf_flamegraph/results/perf.data \
      --stdio \
      > experiments/11_perf_flamegraph/results/perf_report.txt

Generate flamegraph:

    perf script \
      -i experiments/11_perf_flamegraph/results/perf.data \
      > experiments/11_perf_flamegraph/results/perf_script.txt

    tools/FlameGraph/stackcollapse-perf.pl \
      experiments/11_perf_flamegraph/results/perf_script.txt \
      > experiments/11_perf_flamegraph/results/out.folded

    tools/FlameGraph/flamegraph.pl \
      experiments/11_perf_flamegraph/results/out.folded \
      > experiments/11_perf_flamegraph/results/mutex_contention_flamegraph.svg

## Generated Artifacts

- `results/perf.data`
- `results/perf_report.txt`
- `results/perf_script.txt`
- `results/out.folded`
- `results/mutex_contention_flamegraph.svg`

## Benchmark Result During Profiling

The profiled run preserved the same mutex-contention behavior seen in Experiment 03.

At 12 threads:

    Elapsed: 1755 ms
    Throughput: 3.4188e+07 increments/sec
    Throughput vs 1 thread: 0.184615x

This confirms the benchmark still exhibits throughput collapse under contention.

## perf Report Highlights

The perf report showed most sampled time inside the worker thread path:

    95.60% std::thread worker lambda

Important lock-related symbols appeared prominently:

    ___pthread_mutex_lock
    ___pthread_mutex_unlock
    __pthread_mutex_unlock_usercnt
    lll_mutex_unlock_optimized
    __lll_lock_wait
    __lll_lock_wake
    futex_wait

The report also showed significant time in lock wake paths:

    46.94% __lll_lock_wake

## Interpretation

The flamegraph confirms that the mutex contention benchmark is dominated by synchronization overhead.

The hot path is not the counter increment itself.

Instead, time is spent around:

- mutex lock acquisition
- mutex unlock
- futex wait/wake behavior
- thread synchronization paths
- kernel/libpthread locking machinery

This matches the timing results from Experiment 03, where adding more threads reduced total throughput instead of improving it.

## Key Lesson

Timing results identify that performance collapsed.

Profiling explains why it collapsed.

In this case:

    shared mutex contention -> lock/futex hot paths -> poor scalability

The flamegraph turns the benchmark from a timing observation into a bottleneck analysis.

## Why This Matters

Performance engineering requires more than measuring elapsed time.

A useful investigation should connect:

- benchmark behavior
- profiling evidence
- hot-path analysis
- explanation of the bottleneck
- possible design alternatives

This experiment introduces a reproducible profiling workflow for the Phase 2 lab.

