#pragma once
#include <atomic>
#include <optional>
#include <thread>

// https://github.com/bowtoyourlord/MPSCQueue
// Multiple-producer, single-consumer ring-buffer queue.
// Uses reserveWriteIndex for producer-side slot reservation,
// and commitWriteIndex for visibility to the consumer.
// Producer: CAS(reserve) -> write -> CAS(commit)
// Consumer reads only committed slots.
//
// Memory ordering:
//  - Producers publish writes with `release` on commitWriteIndex
//  - Consumer observes committed writes with `acquire`
//
// Invariant: commitWriteIndex >= reserveWriteIndex >= readIndex

const size_t MPSC_QUEUE_CAPACITY = 1024;  // must be power of 2

template <typename T>
class MPSCQueue {
 public:
  MPSCQueue();
  ~MPSCQueue();

  [[nodiscard]] std::optional<T> pop() noexcept;
  [[nodiscard]] bool push(const T& msg) noexcept;

 private:
  static constexpr size_t capacity = MPSC_QUEUE_CAPACITY;
  static constexpr size_t maxIndexMask = capacity - 1;

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> commitWriteIndex{0};
  std::atomic<size_t> reserveWriteIndex{0};

  T* buf = nullptr;
};

template <typename T>
MPSCQueue<T>::MPSCQueue() {
  static_assert((capacity > 0 && ((capacity & (capacity - 1)) == 0)),
                "MPSCQueue algorithms are optimised for and intended to work "
                "only with capacity of power of 2");
  static_assert(std::atomic<size_t>::is_always_lock_free,
                "MPSCQueue requires lock-free atomics for correctness");
  buf = new T[capacity];
}

template <typename T>
MPSCQueue<T>::~MPSCQueue() {
  delete[] buf;
}

template <typename T>
bool MPSCQueue<T>::push(const T& msg) noexcept {
  while (true) {
    size_t rw = reserveWriteIndex.load(std::memory_order_relaxed);
    size_t r = readIndex.load(std::memory_order_relaxed);

    if (rw - r == capacity) {
      // Buffer is full, writer has lapped the reader by full capacity
      return false;
    }

    if (reserveWriteIndex.compare_exchange_weak(rw, rw + 1,
                                                std::memory_order_relaxed)) {
      buf[rw & maxIndexMask] = msg;

      while (!commitWriteIndex.compare_exchange_weak(
          rw, rw + 1, std::memory_order_release, std::memory_order_relaxed)) {
        std::this_thread::yield();
      }
      return true;
    }
  }
}

template <typename T>
std::optional<T> MPSCQueue<T>::pop() noexcept {
  size_t r = readIndex.load(std::memory_order_relaxed);
  size_t cw = commitWriteIndex.load(std::memory_order_acquire);

  if (cw == r) {
    // buffer empty
    return std::nullopt;
  }

  auto msg = buf[r & maxIndexMask];

  readIndex.store(r + 1, std::memory_order_relaxed);

  return std::make_optional(msg);
}
