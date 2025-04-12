// https://www.modernescpp.com/index.php/a-lock-free-stack-a-hazard-pointer-implementation/

#include <atomic>
#include <cstddef>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

template <typename T>
concept Node = requires(T a) {
  { T::data };
  { *a.next } -> std::same_as<T&>;
};

template <typename T>
struct MyNode {
  T data;
  MyNode* next;
  MyNode(T d) : data(d), next(nullptr) {}
};

constexpr std::size_t MaxHazardPointers = 50;

template <typename T, Node MyNode = MyNode<T>>
struct HazardPointer {
  std::atomic<std::thread::id> id;
  std::atomic<MyNode*> pointer;
};

template <typename T>
HazardPointer<T> HazardPointers[MaxHazardPointers];

template <typename T, Node MyNode = MyNode<T>>
class HazardPointerOwner {
  HazardPointer<T>* hazardPointer;

 public:
  HazardPointerOwner(HazardPointerOwner const&) = delete;
  HazardPointerOwner operator=(HazardPointerOwner const&) = delete;

  HazardPointerOwner() : hazardPointer(nullptr) {
    for (std::size_t i = 0; i < MaxHazardPointers; ++i) {
      std::thread::id old_id;
      if (HazardPointers<T>[i].id.compare_exchange_strong(
              old_id, std::this_thread::get_id())) {
        hazardPointer = &HazardPointers<T>[i];
        break;
      }
    }
    if (!hazardPointer) {
      throw std::out_of_range("No hazard pointers available!");
    }
  }

  std::atomic<MyNode*>& getPointer() { return hazardPointer->pointer; }

  ~HazardPointerOwner() {
    hazardPointer->pointer.store(nullptr);
    hazardPointer->id.store(std::thread::id());
  }
};

template <typename T, Node MyNode = MyNode<T>>
std::atomic<MyNode*>& getHazardPointer() {
  thread_local static HazardPointerOwner<T> hazard;
  return hazard.getPointer();
}

template <typename T, Node MyNode = MyNode<T>>
class RetireList {
  struct RetiredNode {
    MyNode* node;
    RetiredNode* next;
    RetiredNode(MyNode* p) : node(p), next(nullptr) {}
    ~RetiredNode() { delete node; }
  };

  std::atomic<RetiredNode*> RetiredNodes;

  void addToRetiredNodes(RetiredNode* retiredNode) {
    retiredNode->next = RetiredNodes.load();
    while (
        !RetiredNodes.compare_exchange_strong(retiredNode->next, retiredNode));
  }

 public:
  bool isInUse(MyNode* node) {
    for (std::size_t i = 0; i < MaxHazardPointers; ++i) {
      if (HazardPointers<T>[i].pointer.load() == node) return true;
    }
    return false;
  }

  void addNode(MyNode* node) { addToRetiredNodes(new RetiredNode(node)); }

  void deleteUnusedNodes() {
    RetiredNode* current = RetiredNodes.exchange(nullptr);
    while (current) {
      RetiredNode* const next = current->next;
      if (!isInUse(current->node))
        delete current;
      else
        addToRetiredNodes(current);
      current = next;
    }
  }
};

template <typename T, Node MyNode = MyNode<T>>
class LockFreeStack {
  std::atomic<MyNode*> head;
  RetireList<T> retireList;

 public:
  LockFreeStack() = default;
  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  void push(T val) {
    MyNode* const newMyNode = new MyNode(val);
    newMyNode->next = head.load();
    while (!head.compare_exchange_strong(newMyNode->next, newMyNode));
  }

  T topAndPop() {
    std::atomic<MyNode*>& hazardPointer = getHazardPointer<T>();
    MyNode* oldHead = head.load();
    do {
      MyNode* tempMyNode;
      do {
        tempMyNode = oldHead;
        hazardPointer.store(oldHead);
        oldHead = head.load();
      } while (oldHead != tempMyNode);
    } while (oldHead && !head.compare_exchange_strong(oldHead, oldHead->next));
    if (!oldHead) throw std::out_of_range("The stack is empty!");
    hazardPointer.store(nullptr);
    auto res = oldHead->data;
    if (retireList.isInUse(oldHead))
      retireList.addNode(oldHead);
    else
      delete oldHead;
    retireList.deleteUnusedNodes();
    return res;
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

  fut.get(), fut1.get(), fut2.get();

  std::cout << fut3.get() << '\n';
  std::cout << fut4.get() << '\n';
  std::cout << fut5.get() << '\n';
}
