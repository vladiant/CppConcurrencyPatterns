// https://medium.com/@simontoth/daily-bit-e-of-c-std-atomic-std-shared-ptr-std-atomic-std-weak-ptr-f4a408611087
#include <atomic>
#include <memory>
#include <optional>

// Example of a simple thread-safe
// and resource-safe stack datastructure
template <typename T>
struct Stack {
  struct Node {
    T value;
    std::shared_ptr<Node> prev;
  };

  void push(T value) {
    // Make a new node, setting the current head as its previous value
    auto active = std::make_shared<Node>(std::move(value), head_.load());

    // Another thread can come in a modify the current head,
    // so check if active->prev is still head_
    // - if it's not, update active->prev to the current value of head_ and loop
    // - if it is, update head_ to active
    while (not head_.compare_exchange_weak(active->prev, active));
  }

  std::optional<T> pop() {
    // Load the current head
    auto active = head_.load();

    // Another thread can come in and modify the current head,
    // so check if the head has changed (i.e. head_ != active)
    // - if it has changed, update active to the current head and loop
    // - if it hasn't changed, update the head to the previous element on the
    // stack
    while (active != nullptr &&
           not head_.compare_exchange_weak(active, active->prev));

    // If we didn't run out of elements, return the value
    if (active != nullptr)
      return {std::move(active->value)};
    else
      return std::nullopt;
  }

 private:
  std::atomic<std::shared_ptr<Node>> head_;
};

#include <array>
#include <ranges>
#include <thread>

int main() {
  Stack<int> stack;
  std::array<std::jthread, 4> writers;
  std::array<std::jthread, 4> readers;

  // start the writers
  for (auto& t : writers)
    t = std::jthread{[&stack] {
      // write 100 values
      for (int i = 0; i < 100; i++) stack.push(42);
    }};
  // start the readers
  for (auto& t : readers)
    t = std::jthread{[&stack] {
      // read 100 values
      for (int i = 0; i < 100; i++)
        while (stack.pop() == std::nullopt);
    }};
}
