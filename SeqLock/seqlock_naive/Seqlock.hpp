#pragma once

#include <atomic>
#include <concepts>

template <typename T>
requires std::is_nothrow_copy_assignable_v<T> &&
    std::is_trivially_copy_assignable_v<T>
class Seqlock {
 public:
  T load() const noexcept {
    T copy;
    for (;;) {
      const auto startFlag = mCounter.load();
      if ((startFlag & 1) == 1) {
        continue;
      }
      copy = mSharedValue;
      const auto endFlag = mCounter.load();
      if (startFlag == endFlag) {
        break;
      }
    }
    return copy;
  }

  void store(const T &aValue) noexcept {
    mCounter++;
    mSharedValue = aValue;
    mCounter++;
  }

 private:
  std::atomic_uint32_t mCounter{0};
  T mSharedValue{};
};
