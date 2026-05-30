// https://dev.to/absterdabster/what-the-futex-a-linux-concurrency-fundamental-989
// sudo perf stat -e
// L1-dcache-loads,L1-dcache-load-misses,mem_inst_retired.lock_loads ./spinlock
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <thread>

void futex_wait(std::atomic_int *addr, int expected) {
  syscall(SYS_futex, addr, FUTEX_WAIT, expected, NULL, NULL, 0);
}

void futex_wake(std::atomic_int *addr, int threads = 1) {
  syscall(SYS_futex, addr, FUTEX_WAKE, threads, NULL, NULL, 0);
}

class SpinLock {
 public:
  void lock() noexcept {
    int unlocked = 0;
    while (!state.compare_exchange_strong(unlocked, 1)) {
      unlocked = 0;
      futex_wait(&state, 1);
    }
  }

  bool try_lock() noexcept {
    // First do a relaxed load to check if lock is free in order to prevent
    // unnecessary cache misses if someone does while(!try_lock())
    return !state.load(std::memory_order_relaxed) &&
           !state.exchange(1, std::memory_order_acquire);
  }

  void unlock() noexcept {
    state.store(0);
    futex_wake(&state);
  }

 private:
  // 0 - unlocked, 1 - locked
  std::atomic_int state{0};
};

SpinLock lock;

void increment_counter(int64_t &counter) {
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
