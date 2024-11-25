// https://github.com/CoffeeBeforeArch/spinlocks/blob/main/spin_locally/spin_locally.cpp

#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <pthread.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace audio {
struct SpinLock {
  void lock() noexcept {
    // approx. 5x5 ns (= 25 ns), 10x40 ns (= 400 ns), and 3000x350 ns
    // (~ 1 ms), respectively, when measured on a 2.9 GHz Intel i9
    constexpr std::array iterations = {5, 10, 3000};

    for (int i = 0; i < iterations[0]; ++i) {
      if (try_lock()) return;
    }

    for (int i = 0; i < iterations[1]; ++i) {
      if (try_lock()) return;

      _mm_pause();
    }

    while (true) {
      for (int i = 0; i < iterations[2]; ++i) {
        if (try_lock()) return;

        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
        _mm_pause();
      }

      // waiting longer than we should, let's give other threads
      // a chance to recover
      std::this_thread::yield();
    }
  }

  bool try_lock() noexcept {
    return !flag.test_and_set(std::memory_order_acquire);
  }

  void unlock() noexcept { flag.clear(std::memory_order_release); }

 private:
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
}  // namespace audio

namespace a_bool {
class SpinLock {
 public:
  void lock() noexcept {
    for (;;) {
      // Optimistically assume the lock is free on the first try
      if (!lock_.exchange(true, std::memory_order_acquire)) {
        return;
      }
      // Wait for lock to be released without generating cache misses
      while (lock_.load(std::memory_order_relaxed)) {
        // Issue X86 PAUSE or ARM YIELD instruction to reduce contention
        // between hyper-threads
        __builtin_ia32_pause();
      }
    }
  }

  bool try_lock() noexcept {
    // First do a relaxed load to check if lock is free in order to prevent
    // unnecessary cache misses if someone does while(!try_lock())
    return !lock_.load(std::memory_order_relaxed) &&
           !lock_.exchange(true, std::memory_order_acquire);
  }

  void unlock() noexcept { lock_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> lock_{false};
};
}  // namespace a_bool

namespace a_flag {
class SpinLock {
 public:
  void lock() noexcept {
    while (mFlag.test_and_set(std::memory_order_acquire)) {
      mFlag.wait(true, std::memory_order_relaxed);
    }
  }

  void unlock() noexcept {
    mFlag.clear(std::memory_order_release);
    mFlag.notify_one();
  }

 private:
  std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
};

SpinLock lock;

void increment_counter(int64_t &counter) {
  for (int i = 0; i < 10000; i++) {
    lock.lock();
    counter++;
    lock.unlock();
  }
}
}  // namespace a_flag

namespace a_flag_c {
class SpinLock {
 public:
  void lock() noexcept {
    while (atomic_flag_test_and_set_explicit(&sl, std::memory_order_acquire)) {
      /* use whatever is appropriate for your target arch here */
      __builtin_ia32_pause();
    }
  }

  void unlock() noexcept {
    atomic_flag_clear_explicit(&sl, std::memory_order_release);
  }

 private:
  std::atomic_flag sl = ATOMIC_FLAG_INIT;
};
}  // namespace a_flag_c

namespace a_posix_c {
class SpinLock {
 public:
  SpinLock() { pthread_spin_init(&sl, PTHREAD_PROCESS_PRIVATE); }

  void lock() noexcept { pthread_spin_lock(&sl); }

  void unlock() noexcept { pthread_spin_unlock(&sl); }

 private:
  pthread_spinlock_t sl;
};
}  // namespace a_posix_c

template <typename T>
void inc(T &s, std::int64_t &val) {
  for (int i = 0; i < 100000; i++) {
    s.lock();
    val++;
    s.unlock();
  }
}

template <typename SpinLockT>
void spinlock_benchmark(benchmark::State &s) {
  auto num_threads = s.range(0);

  std::int64_t val = 0;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  SpinLockT sl;

  for (auto _ : s) {
    for (auto i = 0u; i < num_threads; i++) {
      threads.emplace_back([&] { inc(sl, val); });
    }

    for (auto &thread : threads) thread.join();
    threads.clear();
  }
}

static void audio_spin_mutex(benchmark::State &s) {
  spinlock_benchmark<audio::SpinLock>(s);
}

BENCHMARK(audio_spin_mutex)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

static void spinlock_atomic_bool(benchmark::State &s) {
  spinlock_benchmark<a_bool::SpinLock>(s);
}

BENCHMARK(spinlock_atomic_bool)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

static void spinlock_atomic_flag(benchmark::State &s) {
  spinlock_benchmark<a_flag::SpinLock>(s);
}

BENCHMARK(spinlock_atomic_flag)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

static void spinlock_atomic_flag_c(benchmark::State &s) {
  spinlock_benchmark<a_flag_c::SpinLock>(s);
}

BENCHMARK(spinlock_atomic_flag_c)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

static void spinlock_posix_c(benchmark::State &s) {
  spinlock_benchmark<a_posix_c::SpinLock>(s);
}

BENCHMARK(spinlock_posix_c)
    ->RangeMultiplier(2)
    ->Range(1, std::thread::hardware_concurrency())
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
