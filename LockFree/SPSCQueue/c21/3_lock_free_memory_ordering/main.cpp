// https://david.alvarezrosa.com/posts/optimizing-a-lock-free-ring-buffer/
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

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
class LockfreeSizeAtomicAcqRelSpsc : NonCopyableNonMovable {
  static_assert(std::is_trivial_v<T>, "spsc T must be trivial");
  size_t cap_;
  size_t pushInd_ = 0, popInd_ = 0;
  std::unique_ptr<T[]> buf_;
  std::atomic<uint64_t> size_ = 0;

 public:
  LockfreeSizeAtomicAcqRelSpsc(size_t sz) {
    cap_ = sz;
    buf_ = std::make_unique<T[]>(cap_);
  }

  bool push(const T& t) {
    if (size_.load(std::memory_order_acquire) == cap_) return false;
    buf_[pushInd_++ % cap_] = t;
    size_.fetch_add(1, std::memory_order_release);
    return true;
  }

  bool pop(T& t) {
    if (size_.load(std::memory_order_acquire) == 0) return false;
    t = buf_[popInd_++ % cap_];
    size_.fetch_sub(1, std::memory_order_release);
    return true;
  }
};

// Demo 1: Basic Usage
void demo_basic_usage() {
  std::cout << "=== Demo 1: Basic Usage ===\n";

  LockfreeSizeAtomicAcqRelSpsc<int> queue(10);

  // Push some values
  std::cout << "Pushing values: ";
  for (int i = 1; i <= 5; ++i) {
    if (queue.push(i * 10)) {
      std::cout << i * 10 << " ";
    }
  }
  std::cout << "\n";

  // Pop values
  std::cout << "Popping values: ";
  int value;
  while (queue.pop(value)) {
    std::cout << value << " ";
  }
  std::cout << "\n\n";
}

// Demo 2: Queue Full Scenario
void demo_queue_full() {
  std::cout << "=== Demo 2: Queue Full Scenario ===\n";

  LockfreeSizeAtomicAcqRelSpsc<int> queue(5);  // Small queue

  std::cout << "Attempting to push 10 items into queue of size 5:\n";
  for (int i = 0; i < 10; ++i) {
    if (queue.push(i)) {
      std::cout << "  Pushed: " << i << "\n";
    } else {
      std::cout << "  FAILED to push: " << i << " (queue full)\n";
    }
  }
  std::cout << "\n";
}

// Demo 3: Producer-Consumer Pattern
void demo_producer_consumer() {
  std::cout << "=== Demo 3: Producer-Consumer Pattern ===\n";

  LockfreeSizeAtomicAcqRelSpsc<int> queue(100);
  std::atomic<bool> done{false};

  // Producer thread
  std::thread producer([&queue, &done]() {
    for (int i = 0; i < 50; ++i) {
      while (!queue.push(i)) {
        // Retry if queue is full
        std::this_thread::yield();
      }
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    done.store(true);
  });

  // Consumer thread
  std::thread consumer([&queue, &done]() {
    int value;
    int count = 0;
    std::vector<int> received;

    while (!done.load() || queue.pop(value)) {
      if (queue.pop(value)) {
        received.push_back(value);
        count++;
      } else {
        std::this_thread::yield();
      }
    }

    std::cout << "Consumer received " << count << " items\n";
    std::cout << "First 10 items: ";
    for (int i = 0; i < std::min(10, (int)received.size()); ++i) {
      std::cout << received[i] << " ";
    }
    std::cout << "\n";
  });

  producer.join();
  consumer.join();
  std::cout << "\n";
}

// Demo 4: Performance Measurement
void demo_performance() {
  std::cout << "=== Demo 4: Performance Measurement ===\n";

  const size_t ITEMS = 1000000;
  LockfreeSizeAtomicAcqRelSpsc<int> queue(1024);
  std::atomic<bool> done{false};

  auto start = std::chrono::high_resolution_clock::now();

  std::thread producer([&queue, &done]() {
    for (size_t i = 0; i < ITEMS; ++i) {
      while (!queue.push(static_cast<int>(i))) {
        std::this_thread::yield();
      }
    }
    done.store(true);
  });

  std::thread consumer([&queue, &done]() {
    int value;
    size_t count = 0;

    while (!done.load() || queue.pop(value)) {
      if (queue.pop(value)) {
        count++;
      } else {
        std::this_thread::yield();
      }
    }

    std::cout << "Consumer received " << count << " items\n";
  });

  producer.join();
  consumer.join();

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Transferred " << ITEMS << " items in " << duration.count()
            << " ms\n";
  std::cout << "Throughput: " << (ITEMS * 1000.0 / duration.count())
            << " items/sec\n";
  std::cout << "\n";
}

int main() {
  std::cout << "SPSC Queue Demonstration\n";
  std::cout << "========================\n\n";

  demo_basic_usage();
  demo_queue_full();
  demo_producer_consumer();
  demo_performance();

  std::cout << "All demos completed successfully!\n";

  return 0;
}
