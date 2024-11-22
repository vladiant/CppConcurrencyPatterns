#include <future>
#include <iostream>
#include <thread>

// https://github.com/rigtorp/Seqlock/
#include "rigtorp/Seqlock.hpp"

using rigtorp::Seqlock;

struct Data {
  std::size_t a, b, c;
};

std::atomic_bool exitFlag = false;

std::hash<std::thread::id> hasher;

template <typename T>
std::string reader(Seqlock<T>& aSeqlock) {
  std::string result = std::to_string(hasher(std::this_thread::get_id()));
  for (; !exitFlag;) {
    auto readData = aSeqlock.load();
    if (readData.a + 100 == readData.b &&
        readData.c == readData.a + readData.b) {
      return result + " success";
    }
  }
  return result + " fail";
}

template <typename T>
void writer(Seqlock<T>& aSeqlock) {
  aSeqlock.store({3, 2, 1});
  aSeqlock.store({100, 200, 300});
}

int main() {
  Seqlock<Data> seqlock;

  auto firstReader =
      std::async(std::launch::async, reader<Data>, std::ref(seqlock));
  auto secondReader =
      std::async(std::launch::async, reader<Data>, std::ref(seqlock));

  std::thread writerThread(writer<Data>, std::ref(seqlock));

  writerThread.join();
  exitFlag = true;

  std::cout << firstReader.get() << '\n';
  std::cout << secondReader.get() << '\n';

  return 0;
}
