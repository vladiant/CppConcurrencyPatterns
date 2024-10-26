#include "selector.hpp"

void Selector::select() {
  std::unique_lock<std::mutex> lock(mtx);

  while (!stop_requested()) {
    // Wait until a channel has data available or a stop is requested.
    cv.wait(lock, [this] {
      return stop_requested() ||
             std::any_of(channels.begin(), channels.end(),
                         [](const auto& ch) { return ch(); });
    });

    // Process all channels that have data available
    for (auto ch_it = channels.begin(); ch_it != channels.end();) {
      if ((*ch_it)()) {
        // Data processed, remove this channel from the list
        ch_it = channels.erase(ch_it);
      } else {
        ++ch_it;
      }
    }

    if (channels.empty()) {
      break;
    }
  }
}