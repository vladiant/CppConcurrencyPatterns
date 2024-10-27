#include <atomic>
#include <condition_variable>
#include <iostream>
#include <optional>
#include <thread>

constexpr int kMaxMessagesCount = 10;

constexpr int kBufferSize = 3;

std::atomic_uint messagesCount{0};

struct Portion {
  unsigned int data{};
};

auto produce_next_portion() {
  return Portion{.data = messagesCount.fetch_add(1)};
}

void process_portion_taken([[maybe_unused]] Portion&& portion) {}

class BoundedBuffer {
  Portion buffer[kBufferSize];     // 0..kBufferSize-1
  unsigned head{0}, tail{0};       // 0..kBufferSize-1
  std::atomic<unsigned> count{0};  // 0..kBufferSize
  std::condition_variable nonempty, nonfull;
  std::mutex mtx;
  std::atomic_bool endSignal{false};

 public:
  void append(Portion&& portion) {
    std::unique_lock lck{mtx};
    nonfull.wait(lck, [&] { return !(kBufferSize == count) || endSignal; });
    if (endSignal) {
      return;
    }

    // assert(count < kBufferSize);
    std::cout << std::this_thread::get_id() << "\tAppend: " << portion.data
              << '\n';

    buffer[tail++] = std::move(portion);
    tail %= kBufferSize;

    ++count;

    nonempty.notify_one();
  }

  std::optional<Portion> remove() {
    std::unique_lock lck{mtx};
    nonempty.wait(lck, [&] { return !(0 == count) || endSignal; });
    if (endSignal) {
      return {};
    }

    // assert(count <= kBufferSize);
    Portion portion = buffer[head++];
    std::cout << std::this_thread::get_id() << "\tRemove: " << portion.data
              << '\n';
    head %= kBufferSize;
    --count;

    nonfull.notify_one();

    return portion;
  }

  void stop() {
    endSignal = true;
    nonempty.notify_all();
    nonfull.notify_all();
  }
};

BoundedBuffer buffer;

void producer() {
  for (int i = 0; i < kMaxMessagesCount; i++) {
    Portion portion = produce_next_portion();
    buffer.append(std::move(portion));
  }
}

void consumer() {
  for (;;) {
    auto portion = buffer.remove();
    if (!portion) {
      return;
    }
    process_portion_taken(std::move(*portion));
  }
}

int main() {
  std::jthread t1{producer};
  std::jthread t2{consumer};
  std::jthread t3{producer};
  std::jthread t4{consumer};

  std::this_thread::sleep_for(std::chrono::seconds(2));
  buffer.stop();

  return 0;
}