#include <future>
#include <iostream>
#include <thread>

int main() {
  auto future = std::async(std::launch::async, [] { return "LazyOrEager"; });
  std::cout << future.get() << '\n';

  return 0;
}
