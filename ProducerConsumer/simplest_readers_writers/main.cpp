#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

constexpr int kMaxMessagesCount = 10;

std::atomic_uint messagesCount{0};

std::atomic_bool endSignal{false};

std::condition_variable readerCond;

std::mutex dataGuard;
std::queue<int> data;

void writer(int value) {
  std::unique_lock lock{dataGuard};
  std::cout << std::this_thread::get_id() << "\tWrite: " << value << '\n';
  data.push(value);
  readerCond.notify_one();
}

std::optional<int> reader() {
  std::optional<int> result;

  {
    std::unique_lock lock{dataGuard};
    readerCond.wait(lock, []() { return !data.empty() || endSignal; });
    if (!data.empty()) {
      result = data.front();
      data.pop();
      std::cout << std::this_thread::get_id() << "\tRead:  " << *result << '\n';
    } else {
      std::cout << std::this_thread::get_id() << "\tRead done!\n";
    }
  }

  return result;
}

int main() {
  std::jthread t1{[]() {
    for (int i = 0; i < kMaxMessagesCount; i++) {
      writer(messagesCount.fetch_add(1));
    }
  }};

  std::jthread t2{[]() {
    while (auto result = reader()) {
      // Process result here
    };
  }};

  std::jthread t3{[]() {
    for (int i = 0; i < kMaxMessagesCount; i++) {
      writer(messagesCount.fetch_add(1));
    }
  }};

  std::jthread t4{[]() {
    while (auto result = reader()) {
      // Process result here
    };
  }};

  endSignal = true;

  return 0;
}
