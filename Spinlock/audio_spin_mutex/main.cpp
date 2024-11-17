#include <emmintrin.h>

#include <array>
#include <atomic>
#include <iostream>
#include <thread>

// https://timur.audio/using-locks-in-real-time-audio-processing-safely
struct audio_spin_mutex {
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

audio_spin_mutex lock;

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
