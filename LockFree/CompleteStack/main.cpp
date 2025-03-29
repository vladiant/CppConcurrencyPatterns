// https://www.modernescpp.com/index.php/a-lock-free-stack-a-complete-implementation/
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>

template <typename T>
class LockFreeStack {
 private:
  struct Node {
    T data;
    Node* next;
    Node(T d) : data(d), next(nullptr) {}
  };
  std::atomic<Node*> head;

 public:
  LockFreeStack() = default;
  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  void push(T val) {
    Node* const newNode = new Node(val);
    newNode->next = head.load();
    while (!head.compare_exchange_strong(newNode->next, newNode));
  }

  T topAndPop() {
    Node* oldHead = head.load();  // 1
    while (oldHead &&
           !head.compare_exchange_strong(oldHead, oldHead->next)) {  // 2
      if (!oldHead) throw std::out_of_range("The stack is empty!");  // 3
    }
    return oldHead->data;  // 4
  }
};

int main() {
  LockFreeStack<int> lockFreeStack;

  auto fut = std::async([&lockFreeStack] { lockFreeStack.push(2011); });
  auto fut1 = std::async([&lockFreeStack] { lockFreeStack.push(2014); });
  auto fut2 = std::async([&lockFreeStack] { lockFreeStack.push(2017); });

  auto fut3 =
      std::async([&lockFreeStack] { return lockFreeStack.topAndPop(); });
  auto fut4 =
      std::async([&lockFreeStack] { return lockFreeStack.topAndPop(); });
  auto fut5 =
      std::async([&lockFreeStack] { return lockFreeStack.topAndPop(); });

  fut.get(), fut1.get(), fut2.get();  // 5

  std::cout << fut3.get() << '\n';
  std::cout << fut4.get() << '\n';
  std::cout << fut5.get() << '\n';
}
