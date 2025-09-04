// https://daugaard.org/blog/how-to-make-your-spsc-ring-buffer-do-nothing-more-efficiently/

#include <cstdio>
#include <mutex>
#include <thread>

#include "RingBuffer_v2.hpp"

constexpr int kMaxMessages = 10;

std::atomic_uint messagesCount{0};

struct Message {
  unsigned int data{};
};

Message produceMessage() { return Message{.data = messagesCount.fetch_add(1)}; }

void consumeMessage([[maybe_unused]] const Message& message) {}

constexpr unsigned int N = 4;

RingBuffer ringBuffer;

std::mutex bufferMutex;
Message buffer[N];

std::atomic<unsigned> count{0};

void producer() {
  for (int i = 0; i < kMaxMessages; i++) {
    Message message = produceMessage();

    ringBuffer.Write(message);

    {
      std::lock_guard lock{bufferMutex};
      printf("Produced: %d\n", message.data);
    }
    ringBuffer.FinishWrite();

    count.fetch_add(1, std::memory_order_relaxed);
  }
}

void consumer() {
  for (int i = 0; i < kMaxMessages; i++) {
    Message message = ringBuffer.Read<Message>();
    {
      std::lock_guard lock{bufferMutex};
      printf("Consumed: %d\n", message.data);
    }

    count.fetch_sub(1, std::memory_order_relaxed);
    consumeMessage(message);

    ringBuffer.FinishRead();
  }
}

int main() {
  ringBuffer.Initialize(buffer, sizeof(buffer));

  std::thread t1(producer);
  std::thread t2(consumer);

  t1.join();
  t2.join();

  return EXIT_SUCCESS;
}
