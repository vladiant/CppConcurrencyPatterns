// https://stackoverflow.com/questions/20342691/how-to-implement-a-seqlock-lock-using-c11-atomic-library
#include "SpinLock.hpp"

void SpinLock::lock() noexcept {
  while (mFlag.test_and_set(std::memory_order_acquire)) {
    mFlag.wait(true, std::memory_order_relaxed);
  }
}

void SpinLock::unlock() noexcept {
  mFlag.clear(std::memory_order_release);
  mFlag.notify_one();
}