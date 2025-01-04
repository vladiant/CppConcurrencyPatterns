// no thread shall be allowed to starve
#include <atomic>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

constexpr int kMaxMessagesCount = 10;

std::atomic_uint messagesCount{0};

// controls access (read/write) to the resource.
std::binary_semaphore resource{1};

// for syncing changes to shared variable readcount
std::binary_semaphore rmutex{1};

// FAIRNESS: preserves ordering of requests (signaling must be FIFO)
std::binary_semaphore serviceQueue{1};

std::mutex readCountGuard;
uint32_t readCount{0};

// Can be replaced with std::binary_semaphore dataGuard{1}
// thread sanitizer does not detect data race
// valgrind detects data race
std::mutex dataGuard;
std::queue<int> data;

void startWrite() {
  // wait in line to be serviced
  serviceQueue.acquire();
  // request exclusive access to resource
  resource.acquire();
  // let next in line be serviced
  serviceQueue.release();
}

void endWrite() {
  // release resource access for next reader/writer
  resource.release();
}

void writer(int value) {
  startWrite();

  {
    // Writing is done
    std::lock_guard lock{dataGuard};
    std::cout << std::this_thread::get_id() << "\tWrite: " << value << '\n';
    data.push(value);
  }

  endWrite();
}

void startRead() {
  // wait in line to be serviced
  serviceQueue.acquire();
  // request exclusive access to readcount
  rmutex.acquire();

  {
    std::lock_guard lock{readCountGuard};
    // update count of active readers
    readCount++;
    // if I am the first reader
    if (readCount == 1) {
      // request resource access for readers (writers blocked)
      resource.acquire();
    }
  }

  // let next in line be serviced
  serviceQueue.release();
  // release access to readcount
  rmutex.release();
}

void endRead() {
  // request exclusive access to readcount
  rmutex.acquire();

  {
    std::lock_guard lock{readCountGuard};
    // update count of active readers
    readCount--;
    // if there are no readers left
    if (readCount == 0) {
      // release resource access for all
      resource.release();
    }
  }

  // release access to readcount
  rmutex.release();
}

std::optional<int> reader() {
  std::optional<int> result;

  startRead();

  {
    // Do the Reading
    std::lock_guard lock{dataGuard};
    if (!data.empty()) {
      result = data.front();
      data.pop();
      std::cout << std::this_thread::get_id() << "\tRead:  " << *result << '\n';
    } else {
      std::cout << std::this_thread::get_id() << "\tRead done!\n";
    }
  }

  endRead();

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

  return 0;
}
