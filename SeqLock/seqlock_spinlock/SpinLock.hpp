// https://stackoverflow.com/questions/20342691/how-to-implement-a-seqlock-lock-using-c11-atomic-library
#pragma once

#include <atomic>

class SpinLock {
 public:
  void lock() noexcept;
  void unlock() noexcept;

 private:
  std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
};