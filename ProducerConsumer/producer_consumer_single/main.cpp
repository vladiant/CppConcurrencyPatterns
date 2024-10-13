#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

// Bounded buffer solution for one producer and one consumer.

constexpr int kMaxMessages = 10;

std::atomic_uint messagesCount{0};

struct Message {
  unsigned int data{};
};

Message produceMessage() { return Message{.data = messagesCount.fetch_add(1)}; }

void consumeMessage([[maybe_unused]] const Message& message) {}

constexpr unsigned int N = 4;

std::mutex bufferMutex;
Message buffer[N];

std::atomic<unsigned> count{0};

void producer() {
  unsigned tail{0};
  for (int i = 0; i < kMaxMessages; i++) {
    Message message = produceMessage();

    while (N == count)
      ;  // busy waiting

    {
      std::lock_guard lock{bufferMutex};
      buffer[tail++] = message;
      printf("Produced: %d\n", message.data);
    }

    tail %= N;
    count.fetch_add(1, std::memory_order_relaxed);
  }
}

void consumer() {
  unsigned head{0};
  for (int i = 0; i < kMaxMessages; i++) {
    while (0 == count)
      ;  // busy waiting

    Message message;
    {
      std::lock_guard lock{bufferMutex};
      message = buffer[head++];
      printf("Consumed: %d\n", message.data);
    }

    head %= N;
    count.fetch_sub(1, std::memory_order_relaxed);
    consumeMessage(message);
  }
}

int main() {
  std::thread t1(producer);
  std::thread t2(consumer);

  t1.join();
  t2.join();

  return EXIT_SUCCESS;
}
