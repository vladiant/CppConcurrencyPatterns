#include <iostream>
#include <thread>

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
