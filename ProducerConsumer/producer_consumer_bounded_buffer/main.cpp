#include <atomic>
#include <iostream>
#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

constexpr int kMaxMessagesCount = 10;

constexpr int kBufferSize = 3;

constexpr auto kTimeout = std::chrono::seconds(1);

std::atomic_uint messagesCount{0};

std::counting_semaphore<kBufferSize> numberQueueingPortions{0};
std::counting_semaphore<kBufferSize> numberEmptyPositions{kBufferSize};

std::mutex bufferManipulation;

struct Portion {
  unsigned int data{};
};

std::queue<Portion> buffer;

auto produce_next_portion() {
  return Portion{.data = messagesCount.fetch_add(1)};
}

void add_portion_to_buffer(Portion&& portion) { buffer.emplace(portion); }

std::optional<Portion> take_portion_from_buffer() {
  if (buffer.empty()) {
    return std::nullopt;
  }
  auto portion = buffer.front();
  buffer.pop();
  return portion;
}

void process_portion_taken([[maybe_unused]] Portion&& portion) {}

void producer() {
  for (int i = 0; i < kMaxMessagesCount; i++) {
    Portion portion = produce_next_portion();

    numberEmptyPositions.acquire();
    {
      std::lock_guard lock{bufferManipulation};
      std::cout << std::this_thread::get_id() << "\tProduce: " << portion.data
                << '\n';
      add_portion_to_buffer(std::move(portion));
    }
    numberQueueingPortions.release();
  }
}

void consumer() {
  for (;;) {
    numberQueueingPortions.try_acquire_for(kTimeout);
    Portion portion;
    {
      std::lock_guard lock{bufferManipulation};

      auto result = take_portion_from_buffer();
      if (!result) {
        return;
      }

      portion = *result;
      std::cout << std::this_thread::get_id() << "\tConsume: " << portion.data
                << '\n';
    }
    numberEmptyPositions.release();
    process_portion_taken(std::move(portion));
  }
}

int main() {
  std::jthread t1{producer};
  std::jthread t2{consumer};
  std::jthread t3{producer};
  std::jthread t4{consumer};

  return 0;
}