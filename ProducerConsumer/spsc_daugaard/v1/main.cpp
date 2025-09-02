#include <thread>

#include "RingBuffer_v1.hpp"

constexpr int kMaxMessages = 10;

std::atomic_uint messagesCount{0};

struct Message {
  unsigned int data{};
};

Message produceMessage() { return Message{.data = messagesCount.fetch_add(1)}; }

void consumeMessage([[maybe_unused]] const Message& message) {}

constexpr unsigned int N = 4;

RingBuffer ringBuffer;

Message buffer[N];

std::atomic<unsigned> count{0};

void producer() {
  unsigned tail{0};
  for (int i = 0; i < kMaxMessages; i++) {
    Message message = produceMessage();

    ringBuffer.Write(message);

    tail %= N;
    count.fetch_add(1, std::memory_order_relaxed);
  }

  ringBuffer.FinishWrite();
}

void consumer() {
  unsigned head{0};
  for (int i = 0; i < kMaxMessages; i++) {
    Message message = ringBuffer.Read<Message>();

    head %= N;
    count.fetch_sub(1, std::memory_order_relaxed);
    consumeMessage(message);
  }

  // ringBuffer.FinishRead();
}

int main() {
  // ringBuffer.Initialize(...);

  // ringBuffer.PrepareWrite(...);
  // ringBuffer.PrepareRead(...);

  std::thread t1(producer);
  std::thread t2(consumer);

  t1.join();
  t2.join();

  return EXIT_SUCCESS;
}
