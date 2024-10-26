// https://blog.andreiavram.ro/cpp-channel-thread-safe-container-share-data-threads/
// https://github.com/andreiavrammsd/cpp-channel/

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "channel.hpp"

constexpr int kMaxMessagesCount = 10;

constexpr size_t kBufferSize = 3;

std::atomic_uint messagesCount{0};

struct Portion {
  unsigned int data{};
};

auto produce_next_portion() {
  return Portion{.data = messagesCount.fetch_add(1)};
}

void process_portion_taken([[maybe_unused]] Portion&& portion) {}

msd::channel<Portion> buffer{kBufferSize};

void producer() {
  for (int i = 0; i < kMaxMessagesCount; i++) {
    Portion portion = produce_next_portion();
    buffer << portion;
  }
}

void consumer() {
  for (;;) {
    Portion portion;
    buffer >> portion;
  }
}

int main() {
  std::jthread t1{producer};
  std::jthread t2{consumer};
  std::jthread t3{producer};
  std::jthread t4{consumer};

  return 0;
}