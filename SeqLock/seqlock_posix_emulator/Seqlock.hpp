#pragma once

#include <pthread.h>

#include <concepts>

template <typename T>
requires std::is_nothrow_copy_assignable_v<T> &&
    std::is_trivially_copy_assignable_v<T>
class Seqlock {
 public:
  Seqlock() noexcept { pthread_rwlock_init(&mSharedMutex, nullptr); }

  ~Seqlock() noexcept { pthread_rwlock_destroy(&mSharedMutex); }

  T load() const noexcept {
    T copy;
    {
      pthread_rwlock_rdlock(&mSharedMutex);
      copy = mSharedValue;
      pthread_rwlock_unlock(&mSharedMutex);
    }
    return copy;
  }

  void store(const T &aValue) noexcept {
    pthread_rwlock_wrlock(&mSharedMutex);
    mSharedValue = aValue;
    pthread_rwlock_unlock(&mSharedMutex);
  }

 private:
  mutable pthread_rwlock_t mSharedMutex;
  T mSharedValue{};
};
