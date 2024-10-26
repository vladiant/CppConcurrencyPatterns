// https://jyotinder.substack.com/p/implementing-go-channels-in-cpp
// https://github.com/JyotinderSingh/CppChan/

#include <iostream>
#include <thread>

#include "channel.hpp"

constexpr int kMaxMessagesCount = 10;

constexpr size_t kBufferSize = 3;

std::atomic_uint messagesCount{0};

// printing only
std::mutex printMutex;

struct Portion {
  unsigned int data{};
};

auto produce_next_portion() {
  return Portion{.data = messagesCount.fetch_add(1)};
}

void process_portion_taken([[maybe_unused]] Portion&& portion) {}

Channel<Portion> buffer{kBufferSize};

void producer() {
  for (int i = 0; i < kMaxMessagesCount; i++) {
    Portion portion = produce_next_portion();

    // Printing only
    {
      std::lock_guard lock{printMutex};
      std::cout << std::this_thread::get_id() << "\tProduced: " << portion.data
                << '\n';
    }

    buffer.send(portion);
  }
}

void consumer() {
  for (;;) {
    if (auto portion = buffer.receive()) {
      // Printing only
      {
        std::lock_guard lock{printMutex};
        std::cout << std::this_thread::get_id()
                  << "\tConsumed: " << portion->data << '\n';
      }

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