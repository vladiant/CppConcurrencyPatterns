// https://github.com/rigtorp/Seqlock/

#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

#include "Seqlock.hpp"

constexpr std::size_t kOffset = 100;
constexpr int kThreadCount = 100;
constexpr std::size_t kOperationsCount = 10000000;

// Fuzz test
int main() {
  struct Data {
    std::size_t a, b, c;
  };

  Seqlock<Data> seqLock;
  std::atomic<std::size_t> isReady{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.push_back(std::thread([&seqLock, &isReady]() {
      while (isReady == 0) {
      }

      for (std::size_t i = 0; i < kOperationsCount; ++i) {
        auto copy = seqLock.load();
        if (copy.a + kOffset != copy.b || copy.c != copy.a + copy.b) {
          exit(EXIT_FAILURE);
        }
      }

      isReady--;
    }));
  }

  std::size_t counter = 0;
  while (true) {
    Data data = {counter++, data.a + kOffset, data.b + data.a};
    seqLock.store(data);

    if (counter == 1) {
      isReady += threads.size();
    }

    if (isReady == 0) {
      break;
    }
  }

  for (auto &t : threads) {
    t.join();
  }

  return EXIT_SUCCESS;
}