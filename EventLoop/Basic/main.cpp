#include <iostream>

#include "event_loop.hpp"

int main() {
  {
    EventLoop eventLoop;

    eventLoop.enqueue([] { std::cout << "message from a different thread\n"; });

    std::future<int> result =
        eventLoop.enqueueAsync([](int x, int y) { return x + y; }, 1, 2);

    std::cout << "enqueueSync "
              << eventLoop.enqueueSync(
                     [](const int& x, int&& y, int z) { return x + y + z; }, 1,
                     2, 3)
              << '\n';

    std::cout << "enqueueAsync " << result.get() << '\n';

    std::cout << "prints before or after the message above\n";
  }

  std::cout << "guaranteed to be printed the last\n";
}
