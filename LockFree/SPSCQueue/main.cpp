// https://habr.com/ru/articles/963818/
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

template <typename T, size_t Size>
class SPSCQueue {
 private:
  std::array<T, Size> buffer;
  std::atomic<size_t> writePos{0};
  std::atomic<size_t> readPos{0};

 public:
  bool push(const T& value) {
    size_t currentWrite = writePos.load(std::memory_order_relaxed);
    size_t nextWrite = (currentWrite + 1) % Size;

    // Проверка на полноту очереди
    if (nextWrite == readPos.load(std::memory_order_acquire)) {
      return false;  // Очередь полна
    }

    buffer[currentWrite] = value;

    // Release гарантирует, что запись в buffer видна consumer'у
    writePos.store(nextWrite, std::memory_order_release);
    return true;
  }

  bool pop(T& result) {
    size_t currentRead = readPos.load(std::memory_order_relaxed);

    // Проверка на пустоту очереди
    if (currentRead == writePos.load(std::memory_order_acquire)) {
      return false;  // Очередь пуста
    }

    result = buffer[currentRead];

    // Release синхронизирует с producer
    readPos.store((currentRead + 1) % Size, std::memory_order_release);
    return true;
  }
};

// Demo 1: Basic Usage
void demo_basic_usage() {
  std::cout << "=== Demo 1: Basic Usage ===\n";

  SPSCQueue<int, 10> queue;

  // Push some values
  std::cout << "Pushing values: ";
  for (int i = 1; i <= 5; ++i) {
    if (queue.push(i * 10)) {
      std::cout << i * 10 << " ";
    }
  }
  std::cout << "\n";

  // Pop values
  std::cout << "Popping values: ";
  int value;
  while (queue.pop(value)) {
    std::cout << value << " ";
  }
  std::cout << "\n\n";
}

// Demo 2: Queue Full Scenario
void demo_queue_full() {
  std::cout << "=== Demo 2: Queue Full Scenario ===\n";

  SPSCQueue<int, 5> queue;  // Small queue

  std::cout << "Attempting to push 10 items into queue of size 5:\n";
  for (int i = 0; i < 10; ++i) {
    if (queue.push(i)) {
      std::cout << "  Pushed: " << i << "\n";
    } else {
      std::cout << "  FAILED to push: " << i << " (queue full)\n";
    }
  }
  std::cout << "\n";
}

// Demo 3: Producer-Consumer Pattern
void demo_producer_consumer() {
  std::cout << "=== Demo 3: Producer-Consumer Pattern ===\n";

  SPSCQueue<int, 100> queue;
  std::atomic<bool> done{false};

  // Producer thread
  std::thread producer([&queue, &done]() {
    for (int i = 0; i < 50; ++i) {
      while (!queue.push(i)) {
        // Retry if queue is full
        std::this_thread::yield();
      }
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    done.store(true);
  });

  // Consumer thread
  std::thread consumer([&queue, &done]() {
    int value;
    int count = 0;
    std::vector<int> received;

    while (!done.load() || queue.pop(value)) {
      if (queue.pop(value)) {
        received.push_back(value);
        count++;
      } else {
        std::this_thread::yield();
      }
    }

    std::cout << "Consumer received " << count << " items\n";
    std::cout << "First 10 items: ";
    for (int i = 0; i < std::min(10, (int)received.size()); ++i) {
      std::cout << received[i] << " ";
    }
    std::cout << "\n";
  });

  producer.join();
  consumer.join();
  std::cout << "\n";
}

// Demo 4: Performance Measurement
void demo_performance() {
  std::cout << "=== Demo 4: Performance Measurement ===\n";

  const size_t ITEMS = 1000000;
  SPSCQueue<int, 1024> queue;
  std::atomic<bool> done{false};

  auto start = std::chrono::high_resolution_clock::now();

  std::thread producer([&queue, &done]() {
    for (size_t i = 0; i < ITEMS; ++i) {
      while (!queue.push(static_cast<int>(i))) {
        std::this_thread::yield();
      }
    }
    done.store(true);
  });

  std::thread consumer([&queue, &done]() {
    int value;
    size_t count = 0;

    while (!done.load() || queue.pop(value)) {
      if (queue.pop(value)) {
        count++;
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Transferred " << ITEMS << " items in " << duration.count()
            << " ms\n";
  std::cout << "Throughput: " << (ITEMS * 1000.0 / duration.count())
            << " items/sec\n";
  std::cout << "\n";
}

// Demo 5: Custom Data Types
struct Message {
  int id;
  double value;
  std::array<char, 32> text;

  Message() : id(0), value(0.0), text{} {}
  Message(int i, double v, const char* t) : id(i), value(v), text{} {
    strncpy(text.data(), t, text.size() - 1);
  }
};

void demo_custom_types() {
  std::cout << "=== Demo 5: Custom Data Types ===\n";

  SPSCQueue<Message, 10> queue;

  // Push messages
  queue.push(Message(1, 3.14, "Hello"));
  queue.push(Message(2, 2.71, "World"));
  queue.push(Message(3, 1.41, "SPSC Queue"));

  // Pop and display messages
  Message msg;
  while (queue.pop(msg)) {
    std::cout << "Message[" << msg.id << "]: " << msg.value << ", \""
              << msg.text.data() << "\"\n";
  }
  std::cout << "\n";
}

int main() {
  std::cout << "SPSC Queue Demonstration\n";
  std::cout << "========================\n\n";

  demo_basic_usage();
  demo_queue_full();
  demo_producer_consumer();
  demo_performance();
  demo_custom_types();

  std::cout << "All demos completed successfully!\n";

  return 0;
}
