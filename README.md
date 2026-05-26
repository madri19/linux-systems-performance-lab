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

## Related Project

- https://github.com/madri19/arm-embedded-linux-performance-lab

