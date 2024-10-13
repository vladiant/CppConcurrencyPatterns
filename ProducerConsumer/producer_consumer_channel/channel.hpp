#ifndef CHANNEL_H
#define CHANNEL_H

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <vector>

class Selector;

template <typename T>
class Channel {
 public:
  /**
   * @brief Constructs a Channel object.
   * @param cap The capacity of the channel. If 0, creates an unbuffered
   * channel.
   *
   * Use Case: Create buffered or unbuffered channels for inter-thread
   * communication. Example: Channel<int> ch(5); // Creates a buffered channel
   * with capacity 5 Channel<std::string> ch; // Creates an unbuffered channel
   */
  Channel(size_t cap = 0) : capacity(cap) {}

  // Disable copying and moving
  Channel(const Channel&) = delete;
  Channel& operator=(const Channel&) = delete;
  Channel(Channel&&) = delete;
  Channel& operator=(Channel&&) = delete;

  // Destructor
  ~Channel() {
    close();  // Ensure the channel is closed before destruction
  }

  /**
   * @brief Sends a value to the channel. Blocks if the channel is full
   * (buffered) or if there's no receiver (unbuffered).
   * @param value The value to send.
   * @throws std::runtime_error if the channel is closed.
   *
   * Use Case: Send data to other threads in a blocking manner.
   * Example: ch.send(42);
   */
  void send(const T& value);

  /**
   * @brief Asynchronously sends a value to the channel.
   * @param value The value to send.
   * @return A std::future<void> representing the asynchronous operation.
   *
   * Use Case: Send data without blocking the current thread.
   * Example: auto future = ch.async_send(42);
   *          future.wait(); // Wait for the send to complete if needed
   */
  std::future<void> async_send(const T& value);

  /**
   * @brief Attempts to send a value to the channel without blocking.
   * @param value The value to send.
   * @return true if the value was sent, false otherwise.
   *
   * Use Case: Try to send data without blocking, useful for timeouts or
   * polling. Example: if (ch.try_send(42)) { std::cout << "Sent
   * successfully\n";
   *          }
   */
  bool try_send(const T& value);

  /**
   * @brief Receives a value from the channel. Blocks if the channel is empty.
   * @return An optional containing the received value, or std::nullopt if the
   * channel is closed and empty.
   *
   * Use Case: Receive data from other threads in a blocking manner.
   * Example: auto value = ch.receive();
   *          if (value) {
   *              std::cout << "Received: " << *value << "\n";
   *          }
   */
  std::optional<T> receive();

  /**
   * @brief Asynchronously receives a value from the channel.
   * @return A std::future containing an optional with the received value.
   *
   * Use Case: Receive data without blocking the current thread.
   * Example: auto future = ch.async_receive();
   *          auto value = future.get();
   */
  std::future<std::optional<T>> async_receive();
  /**
   * @brief Attempts to receive a value from the channel without blocking.
   * @return An optional containing the received value, or std::nullopt if the
   * channel is empty.
   *
   * Use Case: Try to receive data without blocking, useful for timeouts or
   * polling. Example:
   * if (auto value = ch.try_receive()) {
   *  std::cout << "Received: " << *value << "\n";
   * }
   */
  std::optional<T> try_receive();

  /**
   * @brief Closes the channel. No more values can be sent after closing.
   *
   * Use Case: Signal that no more values will be sent on this channel.
   * Example: ch.close();
   */
  void close();

  /**
   * @brief Checks if the channel is closed.
   * @return true if the channel is closed, false otherwise.
   *
   * Use Case: Check the channel state before sending or in a loop.
   * Example:
   * while (!ch.is_closed()) {
   *  // Perform operations
   * }
   */
  bool is_closed() const {
    std::unique_lock<std::mutex> lock(mtx);
    return closed;
  }

  /**
   * @brief Checks if the channel is empty.
   * @return true if the channel is empty, false otherwise.
   *
   * Use Case: Check if there are any pending values in the channel.
   * Example:
   * if (!ch.is_empty()) {
   *  auto value = ch.receive();
   * }
   */
  bool is_empty() const {
    std::unique_lock<std::mutex> lock(mtx);
    return queue.empty();
  }

  /**
   * @brief Returns the current number of items in the channel.
   * @return The number of items currently in the channel.
   *
   * Use Case: Check how many items are waiting in the channel.
   * Example: std::cout << "Items in channel: " << ch.size() << "\n";
   */
  size_t size() const {
    std::unique_lock<std::mutex> lock(mtx);
    return queue.size();
  }

 private:
  /**
   * @brief Registers a selector with the channel.
   *
   * @param selector The selector to register.
   * @note This function is called by the Selector class and should not be
   * called directly.
   * @see Selector
   */
  void register_selector(Selector* selector);

  /**
   * @brief Unregisters a selector from the channel.
   *
   * @param selector The selector to unregister.
   * @note This function is called by the Selector class and should not be
   * called directly.
   * @see Selector
   */
  void unregister_selector(Selector* selector);

  std::queue<T> queue;
  mutable std::mutex mtx;
  std::condition_variable cv_send, cv_recv;
  bool closed = false;
  size_t capacity;
  size_t waitingReceivers = 0;

  friend class Selector;
  std::vector<Selector*> selectors;
};

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

#include "channel.cpp"

#endif  // CHANNEL_H