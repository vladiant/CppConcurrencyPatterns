// readers-preference: no reader shall be kept waiting if the share is currently
// opened for reading
#include <atomic>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

constexpr int kMaxMessagesCount = 10;

std::atomic_uint messagesCount{0};

std::binary_semaphore resource{1};
std::binary_semaphore rmutex{1};

std::mutex countGuard;
uint32_t readcount{0};

std::mutex dataGuard;
std::queue<int> data;

void writer(int value) {
  // Lock the shared file for a writer
  resource.acquire();
  {
    // Writing is done
    std::lock_guard lock{dataGuard};
    std::cout << std::this_thread::get_id() << "\tWrite: " << value << '\n';
    data.push(value);
  }
  // Release the shared file for use by other readers. Writers are allowed if
  // there are no readers requesting it.
  resource.release();
}

void startRead() {
  // Ensure that no other reader can execute the <Entry> section while you are
  // in it
  rmutex.acquire();
  {
    std::lock_guard lock{countGuard};
    // Indicate that you are a reader trying to enter the Critical Section
    readcount++;

    // Checks if you are the first reader trying to enter CS
    if (readcount == 1) {
      // If you are the first reader, lock the resource from writers. Resource
      // stays reserved for subsequent readers
      resource.acquire();
    }
  }
  rmutex.release();
}

void stopRead() {
  // Ensure that no other reader can execute the <Exit> section while you are in
  // it
  rmutex.acquire();
  {
    std::lock_guard lock{countGuard};
    // Indicate that you no longer need the shared resource. One fewer reader
    readcount--;
    // Checks if you are the last (only) reader who is reading the shared file
    if (readcount == 0) {
      // If you are last reader, then you can unlock the resource. This makes it
      // available to writers.
      resource.release();
    }
  }
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

  stopRead();

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
  t3.join();

  std::jthread t4{[]() {
    while (auto result = reader()) {
      // Process result here
    };
  }};

  return 0;
}
