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

void process_portion_taken([[maybe_unused]] const Portion& portion) {}

// Thread sanitizers detect unlock attempts without locks held
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

    try {
      buffer << portion;
    } catch (msd::closed_channel& e) {
      return;
    }
  }
}

void consumer() {
  for (const auto& portion : buffer) {
    // Printing only
    {
      std::lock_guard lock{printMutex};
      std::cout << std::this_thread::get_id() << "\tConsumed: " << portion.data
                << '\n';
    }

    process_portion_taken(std::move(portion));
  }
}

int main() {
  std::jthread t1{producer};
  std::jthread t2{consumer};
  std::jthread t3{producer};
  std::jthread t4{consumer};

  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Printing only
  {
    std::lock_guard lock{printMutex};
    std::cout << "Close channel\n";
    buffer.close();
  }

  return 0;
}