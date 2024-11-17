// https://rigtorp.se/spinlock/
// sudo perf stat -e
// L1-dcache-loads,L1-dcache-load-misses,mem_inst_retired.lock_loads ./spinlock
#include <iostream>
#include <thread>

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
        // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
        // hyper-threads
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

SpinLock lock;

void increment_counter(int64_t& counter) {
  for (int i = 0; i < 10000; i++) {
    lock.lock();
    counter++;
    lock.unlock();
  }
}

int main() {
  int64_t counter = 0;

  std::thread thread1 = std::thread(increment_counter, std::ref(counter));
  std::thread thread2 = std::thread(increment_counter, std::ref(counter));

  thread1.join();
  thread2.join();

  std::cout << "Sum: " << counter << std::endl;

  return 0;
}
