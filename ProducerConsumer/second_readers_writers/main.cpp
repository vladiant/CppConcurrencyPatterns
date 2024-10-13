// writers-preference: no writer, once added to the queue, shall be kept waiting
// longer than absolutely necessary
#include <atomic>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

constexpr int kMaxMessagesCount = 10;

std::atomic_uint messagesCount{0};

std::binary_semaphore wmutex{1};
std::binary_semaphore rmutex{1};
std::binary_semaphore readTry{1};
std::binary_semaphore resource{1};

std::mutex readCountGuard;
uint32_t readCount{0};

std::mutex writeCountGuard;
uint32_t writeCount{0};

std::mutex dataGuard;
std::queue<int> data;

void startWrite() {
  // reserve entry section for writers - avoids race conditions
  wmutex.acquire();

  {
    std::lock_guard lock{writeCountGuard};
    // report yourself as a writer entering
    writeCount++;
    // checks if you're first writer
    if (writeCount == 1) {
      // if you're first, then you must lock the readers out. Prevent them from
      // trying to enter CS
      readTry.acquire();
    }
  }

  // release entry section
  wmutex.release();

  // reserve the resource for yourself - prevents other writers from
  // simultaneously editing the shared resource
  resource.acquire();
}

void endWrite() {
  // reserve exit section
  wmutex.acquire();

  {
    std::lock_guard lock{writeCountGuard};
    // indicate you're leaving
    writeCount--;
    // checks if you're the last writer
    if (writeCount == 0) {
      // if you're last writer, you must unlock the readers. Allows them to try
      // enter CS for reading
      readTry.release();
    }
  }

  wmutex.release();
}

void writer(int value) {
  startWrite();

  {
    // Writing is done
    std::lock_guard lock{dataGuard};
    std::cout << std::this_thread::get_id() << "\tWrite: " << value << '\n';
    data.push(value);
  }
  resource.release();

  endWrite();
}

void startRead() {
  // Indicate a reader is trying to enter
  readTry.acquire();
  // lock entry section to avoid race condition with other readers
  rmutex.acquire();

  {
    std::lock_guard lock{readCountGuard};
    // report yourself as a reader
    readCount++;
    // checks if you are first reader
    if (readCount == 1) {
      // if you are first reader, lock the resource
      resource.acquire();
    }
  }

  // release entry section for other readers
  rmutex.release();
  // indicate you are done trying to access the resource
  readTry.release();
}

void endRead() {
  // reserve exit section - avoids race condition with readers
  rmutex.acquire();

  {
    std::lock_guard lock{readCountGuard};
    // indicate you're leaving
    readCount--;
    // checks if you are last reader leaving
    if (readCount == 0) {
      // if last, you must release the locked resource
      resource.release();
    }
  }

  // release exit section for other readers
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
