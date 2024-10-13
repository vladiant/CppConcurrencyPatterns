// https://jyotinder.substack.com/p/implementing-go-channels-in-cpp
// https://github.com/JyotinderSingh/CppChan/

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "channel.hpp"

constexpr size_t kBufferSize = 3;

std::atomic_uint messagesCount{0};

struct Portion {
  unsigned int data{};
};

auto produce_next_portion() {
  return Portion{.data = messagesCount.fetch_add(1)};
}

void process_portion_taken([[maybe_unused]] Portion&& portion) {}

Channel<Portion> buffer{kBufferSize};

void producer() {
  for (;;) {
    Portion portion = produce_next_portion();
    buffer.send(portion);
  }
}

void consumer() {
  for (;;) {
    if (auto portion = buffer.receive()) {
      process_portion_taken(std::move(*portion));
    }
  }
}

int main() {
  std::jthread t1{producer};
  std::jthread t2{consumer};
  std::jthread t3{producer};
  std::jthread t4{consumer};

  return 0;
}