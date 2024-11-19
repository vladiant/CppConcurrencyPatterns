#pragma once

#include <concepts>
#include <shared_mutex>

template <typename T>
requires std::is_nothrow_copy_assignable_v<T> &&
    std::is_trivially_copy_assignable_v<T>
class Seqlock {
 public:
  T load() const noexcept {
    T copy;
    {
      std::shared_lock lock(mSharedMutex);
      copy = mSharedValue;
    }
    return copy;
  }

  void store(const T &aValue) noexcept {
    std::lock_guard file_lock(mSharedMutex);
    mSharedValue = aValue;
  }

 private:
  mutable std::shared_mutex mSharedMutex;
  T mSharedValue;
};
