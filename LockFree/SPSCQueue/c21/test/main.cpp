// https://blog.c21-mac.com/posts/spsc/
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

class NonCopyableNonMovable {
 public:
  NonCopyableNonMovable() = default;
  ~NonCopyableNonMovable() = default;

  NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
  NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
};

template <typename T>
class LockfreeSizeAtomicSeqCstSpsc : NonCopyableNonMovable {
  static_assert(std::is_trivial_v<T>, "spsc T must be trivial");
  size_t cap_;
  size_t pushInd_ = 0, popInd_ = 0;
  std::unique_ptr<T[]> buf_;
  std::atomic<uint64_t> size_ = 0;

 public:
  LockfreeSizeAtomicSeqCstSpsc(size_t sz) {
    cap_ = sz;
    buf_ = std::make_unique<T[]>(cap_);
  }

  bool push(const T& t) {
    if (size_ == cap_) return false;
    buf_[pushInd_++ % cap_] = t;
    size_++;
    return true;
  }

  bool pop(T& t) {
    if (size_ == 0) return false;
    t = buf_[popInd_++ % cap_];
    size_--;
    return true;
  }
};

template <template <typename> typename spsc>
void test() {
  spsc<int64_t> q(100);
  int pushPopCnt = 1e6;

  std::atomic<bool> start = false;

  auto consumer = std::thread([&start, &q, pushPopCnt]() {
    while (!start) {
    }
    int i = 0;
    auto counter = pushPopCnt;
    while (counter--) {
      int64_t t;
      while (!q.pop(t));
      if (t != i) {
        std::cerr << "Assert failed\n";
        std::cerr << i << " " << t << "\n";
        exit(1);
      }
      i++;
    }
  });

  auto producer = std::thread([&start, &q, pushPopCnt]() {
    while (!start) {
    }
    int i = 0;
    auto counter = pushPopCnt;
    while (counter--) {
      while (!q.push(i));
      i++;
    }
  });

  start = true;
  consumer.join();
  producer.join();
}

int main() {
  std::cout << "SPSC Queue Demonstration\n";
  std::cout << "========================\n\n";

  test<LockfreeSizeAtomicSeqCstSpsc>();

  std::cout << "All demos completed successfully!\n";

  return 0;
}
