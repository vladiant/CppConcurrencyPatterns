#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>

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
  Portion buffer[kBufferSize];  // 0..kBufferSize-1
  unsigned head{0}, tail{0};    // 0..kBufferSize-1
  unsigned count{0};            // 0..kBufferSize
  std::condition_variable nonempty, nonfull;
  std::mutex mtx;

 public:
  void append(Portion&& portion) {
    std::unique_lock<std::mutex> lck(mtx);
    nonfull.wait(lck, [&] { return !(kBufferSize == count); });

    // assert(count < kBufferSize);
    std::cout << std::this_thread::get_id() << "\tAppend: " << portion.data
              << '\n';

    buffer[tail++] = std::move(portion);
    tail %= kBufferSize;

    ++count;

    nonempty.notify_one();
  }

  Portion remove() {
    std::unique_lock<std::mutex> lck(mtx);
    nonempty.wait(lck, [&] { return !(0 == count); });

    // assert(count <= kBufferSize);
    Portion portion = buffer[head++];
    std::cout << std::this_thread::get_id() << "\tRemove: " << portion.data
              << '\n';
    head %= kBufferSize;
    --count;

    nonfull.notify_one();

    return portion;
  }
};

BoundedBuffer buffer;

void producer() {
  for (;;) {
    Portion portion = produce_next_portion();
    buffer.append(std::move(portion));
  }
}

void consumer() {
  for (;;) {
    auto portion = buffer.remove();
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