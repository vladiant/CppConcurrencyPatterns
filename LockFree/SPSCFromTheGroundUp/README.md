# Adapted Code for "SPSC Lock-free, Wait-free Fifo from the Ground Up" Charles Frasch presentation at CPPCON 2023

This project is based on the source code from:
https://github.com/CharlesFrasch/cppcon2023

Presentation:
"Single Producer Single Consumer Lock-free FIFO From the Ground Up - Charles Frasch - CppCon 2023"

## Build With Conan

Install baseline tooling:

    sudo apt install git
    sudo apt install cmake
    sudo apt install python3-pip
    pip install --user conan

Configure Conan profile once:

    conan profile detect --force

Install dependencies and generate CMake toolchain/deps:

    conan install . --output-folder=build/conan --build=missing -s build_type=Debug -s compiler.cppstd=23

Configure and build:

    cmake --preset conan-debug
    cmake --build --preset conan-debug -j

Run tests:

    ctest --test-dir build/conan/build/Debug --output-on-failure

And these if you want to run `perf stat`:

    sudo apt install linux-tools-5.15.90.1-microsoft-standard-WSL2
    sudo apt install linux-tools-standard-WSL2

## Source File Guide

Recommended reading order (compact):
1. `Fifo1.hpp`
2. `Fifo2.hpp`
3. `Fifo3.hpp`
4. `Fifo4.hpp`
5. `Fifo4a.hpp` and `Fifo4b.hpp`
6. `Fifo5.hpp` and `Fifo5a.hpp`
7. `bench.hpp`, `bench.cpp`, `bench_all.cpp`
8. `unitTests.cpp`
9. `rigtorp.hpp` and `boost_lockfree.cpp`

- `CMakeLists.txt`: Defines all executables (FIFO stages, TSAN variants, benchmarks, unit tests) and links external dependencies.
- `conanfile.py`: Conan 2 recipe for Boost, Google Benchmark, and GoogleTest plus CMake toolchain/deps generation.
- `run_all.sh`: Convenience runner for the per-implementation benchmark executables.

- `bench.hpp`: Core benchmark harness (thread pinning, warmup, timed push/pop loop, ops/sec reporting).
- `bench.cpp`: Google Benchmark microbenchmarks for selected FIFO implementations.
- `bench_all.cpp`: Multi-implementation benchmark driver that prints CSV rows for repeated comparison runs.

- `unitTests.cpp`: GoogleTest coverage for FIFO contract semantics, wrap-around behavior, and Fifo5 proxy/value-size traits behavior.

- `Fifo1.hpp`: Stage 1 circular buffer baseline, non-thread-safe and intentionally race-prone.
- `fifo1.cpp`: Minimal executable that benchmarks `Fifo1` through the shared harness.

- `Fifo2.hpp`: Stage 2 introduces atomic cursors for SPSC, but remains a flawed transitional design.
- `fifo2.cpp`: Minimal executable that benchmarks `Fifo2`.

- `Fifo3.hpp`: Stage 3 lock-free SPSC design with acquire/release ordering and cache-line separation to reduce false sharing.
- `fifo3.cpp`: Minimal executable that benchmarks `Fifo3`.

- `Fifo4.hpp`: Stage 4 adds producer/consumer cached cursors to reduce atomic traffic on full/empty checks.
- `fifo4.cpp`: Minimal executable that benchmarks `Fifo4`.

- `Fifo4a.hpp`: Stage 4a variant uses power-of-two masking (`cursor & mask`) instead of modulo indexing.
- `fifo4a.cpp`: Minimal executable that benchmarks `Fifo4a`.

- `Fifo4b.hpp`: Stage 4b variant uses constrained wrap-around cursors (ring-space indexing) rather than monotonic modulo cursors.
- `fifo4b.cpp`: Minimal executable that benchmarks `Fifo4b`.

- `Fifo5.hpp`: Stage 5 builds on Fifo4 with trivial-type requirement, optional ValueSizeTraits copy optimization, and RAII `pusher_t`/`popper_t` proxies.
- `fifo5.cpp`: Minimal executable that benchmarks `Fifo5`.

- `Fifo5a.hpp`: Stage 5a variant combines Fifo5 proxy model with mask-based indexing.
- `fifo5a.cpp`: Minimal executable that benchmarks `Fifo5a`.

- `Mutex.hpp`: Lock-based FIFO baseline using `std::mutex` and blocking lock acquisition.
- `mutex.cpp`: Minimal executable that benchmarks the mutex baseline.

- `TryLock.hpp`: Lock-based FIFO baseline using `std::try_to_lock` semantics.
- `tryLock.cpp`: Minimal executable that benchmarks the try-lock baseline.

- `rigtorp.hpp`: Vendored/modified `rigtorp::SPSCQueue` implementation used as an external lock-free comparison point.
- `rigtorp.cpp`: Minimal executable that benchmarks `rigtorp::SPSCQueue`.

- `boost_lockfree.cpp`: Benchmark adapter for `boost::lockfree::spsc_queue` configured in fixed-size mode.

- `LICENSE`: Project license terms.
