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

// printing only
std::mutex printMutex;

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

    // Printing only
    {
      std::lock_guard lock{printMutex};
      std::cout << std::this_thread::get_id() << "\tProduced: " << portion.data
                << '\n';
    }

    buffer << portion;
  }
}

void consumer() {
  for (;;) {
    Portion portion;
    buffer >> portion;

    // Printing only
    {
      std::lock_guard lock{printMutex};
      std::cout << std::this_thread::get_id() << "\tConsumed: " << portion.data
                << '\n';
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