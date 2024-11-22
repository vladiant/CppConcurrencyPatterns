// https://stackoverflow.com/questions/20342691/how-to-implement-a-seqlock-lock-using-c11-atomic-library
#pragma once

#include <atomic>
#include <concepts>

#include "SpinLock.hpp"

template <typename T>
requires std::is_nothrow_copy_assignable_v<T> &&
    std::is_trivially_copy_assignable_v<T>
class Seqlock {
 public:
  T load() const noexcept {
    T copy;
    uintptr_t flag;
    do {
      read_enter(flag);
      copy = mSharedValue;
    } while (!read_leave(flag));
    return copy;
  }

  void store(const T& aValue) noexcept {
    write_lock();
    mSharedValue = aValue;
    write_unlock();
  }

 private:
  void write_lock() noexcept {
    mLock.lock();
    mFlags.fetch_add(1, std::memory_order_acquire);
  }

  void write_unlock() noexcept {
    mFlags.fetch_add(1, std::memory_order_release);
    mLock.unlock();
  }

  void read_enter(uintptr_t& flag) const noexcept {
    for (;;) {
      const auto f = mFlags.load(std::memory_order_acquire);
      if ((f & 1) == 0) {
        flag = f;
        break;
      }
    }
  }

  bool read_leave(uintptr_t flag) const noexcept {
    atomic_thread_fence(std::memory_order_acquire);
    const auto f = mFlags.load(std::memory_order_relaxed);
    return f == flag;
  }

  std::atomic_uintptr_t mFlags;
  SpinLock mLock;
  T mSharedValue{};
};
