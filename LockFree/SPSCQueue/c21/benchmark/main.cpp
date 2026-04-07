// https://blog.c21-mac.com/posts/spsc/
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#ifdef __x86_64__
#include <immintrin.h>
#include <x86intrin.h>
#else
inline void _mm_pause() {}
#endif

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

inline void pin(int core) {
#ifdef __linux__
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
  (void)core;
#endif
}

inline int64_t curTime() {
#ifdef __x86_64__
  return __rdtsc();
#else
  return std::chrono::steady_clock::now().time_since_epoch().count();
#endif
}

class NonCopyableNonMovable {
 public:
  NonCopyableNonMovable() = default;
  ~NonCopyableNonMovable() = default;

  NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
  NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
};

template <typename T>
class MutexSpsc : NonCopyableNonMovable {
  static_assert(std::is_trivial_v<T>, "spsc T must be trivial");
  size_t cap_;
  size_t pushInd_ = 0, popInd_ = 0;
  std::unique_ptr<T[]> buf_;
  std::mutex mut_;

 public:
  MutexSpsc(size_t sz) {
    cap_ = sz;
    buf_ = std::make_unique<T[]>(cap_);
  }

  bool push(const T& t) {
    std::lock_guard<std::mutex> lck(mut_);
    if (pushInd_ - popInd_ == cap_) return false;
    buf_[pushInd_++ % cap_] = t;
    return true;
  }

  bool pop(T& t) {
    std::lock_guard<std::mutex> lck(mut_);
    if (pushInd_ - popInd_ == 0) return false;
    t = buf_[popInd_++ % cap_];
    return true;
  }
};

template <template <typename> typename spsc, bool UsePause>
double benchmarkThroughput(int consumerCore, int producerCore, size_t queueSize,
                           int64_t iterations) {
  std::align_val_t alignment{std::max<size_t>(alignof(spsc<int64_t>), 64)};
  std::unique_ptr<spsc<int64_t>> qUniquePtr(new (alignment)
                                                spsc<int64_t>(queueSize));
  auto& q = *qUniquePtr;

  std::atomic<bool> start = false;

  // Consumer thread
  auto consumer = std::thread([&]() {
    pin(consumerCore);
    while (!start);  // Wait for start signal

    for (int64_t i = 0; i < iterations; ++i) {
      int64_t val;
      while (!q.pop(val)) {
        if (UsePause) _mm_pause();
      }
      [[maybe_unused]] volatile int64_t v = val;  // Prevent optimization
    }
  });

  // Producer thread
  auto producer = std::thread([&]() {
    pin(producerCore);
    while (!start);  // Wait for start signal

    for (int64_t i = 0; i < iterations; ++i) {
      while (!q.push(i)) {
        if (UsePause) _mm_pause();
      }
    }
  });

  // Start timing
  auto startTime = std::chrono::steady_clock::now();
  start = true;

  // Wait for completion
  consumer.join();
  producer.join();

  // Stop timing
  auto stopTime = std::chrono::steady_clock::now();

  // Calculate ops/second
  auto elapsed_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime)
          .count();
  return (iterations * 1000000000.0) / elapsed_ns;  // ops per second
}

template <template <typename> typename spsc, bool UsePause>
std::vector<uint64_t> benchmarkLatency(int consumerCore, int producerCore,
                                       size_t queueSize, int64_t iterations) {
  std::align_val_t alignment{std::max<size_t>(alignof(spsc<int64_t>), 64)};
  std::unique_ptr<spsc<int64_t>> q1UniquePtr(new (alignment)
                                                 spsc<int64_t>(queueSize));
  auto& q1 = *q1UniquePtr;

  std::atomic<bool> producerTurn = false;
  std::vector<uint64_t> latencies(iterations, 0);

  // Write one entry then wait for consumer to consume. repeat this iterations
  // time.
  auto consumer = std::thread([&]() {
    pin(consumerCore);
    for (int64_t i = 0; i < iterations; ++i) {
      int64_t val;
      while (!q1.pop(val)) {
        if (UsePause) _mm_pause();
      }
      latencies[i] = curTime() - val;
      producerTurn = true;
    }
  });

  auto producer = std::thread([&]() {
    pin(producerCore);
    for (int64_t i = 0; i < iterations; ++i) {
      while (!producerTurn) {
      }
      producerTurn = false;
      while (!q1.push(curTime())) {
      }
    }
  });

  producerTurn = true;

  consumer.join();
  producer.join();

  return latencies;
}

int main() {
  constexpr int consumerCore = 0;
  constexpr int producerCore = 1;
  constexpr size_t queueSize = 1024;
  constexpr int64_t iterations = 1'000'000;

  std::cout << "SPSC Queue Benchmark\n";
  std::cout << "====================\n";
  std::cout << "Queue size: " << queueSize << ", Iterations: " << iterations
            << "\n\n";

  // Throughput benchmark
  std::cout << "--- Throughput (MutexSpsc, no pause) ---\n";
  double opsPerSec = benchmarkThroughput<MutexSpsc, false>(
      consumerCore, producerCore, queueSize, iterations);
  std::cout << "Ops/sec: " << opsPerSec << "\n\n";

  std::cout << "--- Throughput (MutexSpsc, with pause) ---\n";
  double opsPerSecPause = benchmarkThroughput<MutexSpsc, true>(
      consumerCore, producerCore, queueSize, iterations);
  std::cout << "Ops/sec: " << opsPerSecPause << "\n\n";

  // Latency benchmark
  std::cout << "--- Latency (MutexSpsc, no pause) ---\n";
  auto latencies = benchmarkLatency<MutexSpsc, false>(
      consumerCore, producerCore, queueSize, iterations);
  std::sort(latencies.begin(), latencies.end());
  std::cout << "Median:  " << latencies[latencies.size() / 2] << " cycles\n";
  std::cout << "P99:     " << latencies[latencies.size() * 99 / 100]
            << " cycles\n";
  std::cout << "P99.9:   " << latencies[latencies.size() * 999 / 1000]
            << " cycles\n\n";

  std::cout << "--- Latency (MutexSpsc, with pause) ---\n";
  auto latenciesPause = benchmarkLatency<MutexSpsc, true>(
      consumerCore, producerCore, queueSize, iterations);
  std::sort(latenciesPause.begin(), latenciesPause.end());
  std::cout << "Median:  " << latenciesPause[latenciesPause.size() / 2]
            << " cycles\n";
  std::cout << "P99:     " << latenciesPause[latenciesPause.size() * 99 / 100]
            << " cycles\n";
  std::cout << "P99.9:   " << latenciesPause[latenciesPause.size() * 999 / 1000]
            << " cycles\n";

  return 0;
}
