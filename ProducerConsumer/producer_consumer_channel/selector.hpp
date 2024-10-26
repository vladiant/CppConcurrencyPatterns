#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

template <typename T>
class Channel;

/**
 * @brief A Selector class for non-blocking channel operations.
 *
 * The Selector class allows multiple Channel objects to be monitored
 * simultaneously for incoming messages. It provides a select() method that
 * blocks until a message is received on any of the registered channels.
 *
 */
class Selector {
 public:
  Selector() : stop_flag_(false) {}
  // Disable copying and moving
  Selector(const Selector&) = delete;
  Selector& operator=(const Selector&) = delete;
  Selector(Selector&&) = delete;
  Selector& operator=(Selector&&) = delete;

  // Destructor
  ~Selector() = default;

  /**
   * @brief Adds a channel to the selector for receiving messages.
   *
   * @tparam T the type of the channel.
   * @param ch The channel to add.
   * @param callback The callback function to call when a message is received.
   *
   * Use Case: Add a channel to the selector and specify a callback function
   * to be called when a message is received.
   */
  template <typename T>
  void add_receive(Channel<T>& ch, std::function<void(T)> callback);

  /**
   * @brief Continuously processes events on registered channels until
   * signaled to stop.
   *
   * This method runs a loop that:
   * 1. Waits for data to become available on any channel or for a stop
   * signal.
   * 2. Processes all available data once woken up.
   * 3. Removes channels that have been processed.
   * The loop continues until either all channels are processed or a stop is
   * requested.
   *
   * Usage example:
   * @code
   * std::thread selector_thread([&]() {
   *     selector.select();
   * });
   *
   * // Do other work...
   *
   * selector.stop();
   * @endcode
   */
  void select();

  /**
   * @brief Stops the select operation and unblocks the select() method.
   *
   * Use Case: Stop the selector and unblock the select() method.
   * Example: selector.stop();
   */
  void stop() {
    stop_flag_.test_and_set(std::memory_order_relaxed);
    notify();
  }

  /**
   * @brief Notifies the selector that data may be available on the channels.
   *
   * Use Case: Notify the selector that data may be available on the channels.
   * Example: selector.notify();
   * @note This function is meant for internal use and should not be called
   * directly (unless you know what you're doing).
   */
  void notify() { cv.notify_all(); }

 private:
  /**
   * @brief Checks if a stop has been requested.
   *
   * @return true
   * @return false
   */
  bool stop_requested() const {
    return stop_flag_.test(std::memory_order_relaxed);
  }

  std::vector<std::function<bool()>> channels;
  std::atomic_flag stop_flag_;
  std::mutex mtx;
  std::condition_variable cv;
};

template <typename T>
void Selector::add_receive(Channel<T>& ch, std::function<void(T)> callback) {
  std::unique_lock<std::mutex> lock(mtx);
  ch.register_selector(this);
  // Add a lambda function to the channels list
  channels.push_back([&ch, callback = std::move(callback), this]() mutable {
    if (ch.is_closed()) {
      ch.unregister_selector(this);
      return true;  // Signal that this channel is done
    }
    auto value = ch.try_receive();
    if (value) {
      callback(*value);  // Call the callback with the received value
      return false;
    }
    return false;
  });
}