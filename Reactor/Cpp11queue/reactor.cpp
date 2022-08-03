#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

namespace reactor {
struct Message {
  int32_t type;
  char data[256];
};

// Define a few message types to simulate User event behaviour.
enum class MessageTypes : int32_t {
  OPEN = 0,
  READ,
  WRITE,
  CLOSE,
  REGISTER_HANDLER,
  REMOVE_HANDLER
};

class EventHandler {
 public:
  explicit EventHandler(const Message& msg) {
    OnMessage(msg);
    event_loop_start = true;
    reactor_thread = std::thread(&EventHandler::MessageLoop, this);
  };  // Take in the pointer to a message as an argument to the constructor.

  bool RegisterEventHandler(int16_t user_id, int32_t message_type,
                            std::function<void(void)> func) {
    std::lock_guard<std::mutex> lock(map_mutex);

    handler_map[{user_id, message_type}] = func;
    register_status_map[{user_id, message_type}] = true;
    return register_status_map[{user_id, message_type}];
  }

  bool RemoveEventHandler(int16_t user_id, int32_t message_type) {
    std::lock_guard<std::mutex> lock(map_mutex);

    handler_map.erase({user_id, message_type});
    register_status_map[{user_id, message_type}] = false;
    return register_status_map[{user_id, message_type}];
  }

  void OnMessage(const Message& msg) {
    std::unique_lock<std::mutex> lock(msg_mutex);
    msg_queue.push(msg);
    msg_cond.notify_one();
  }

  void startEventHandlerThread() {
    if (event_loop_start == false) {
      event_loop_start = true;
      reactor_thread
          .join();  // Thread was already started and stopped. Restart it again.
      reactor_thread = std::thread(&EventHandler::MessageLoop, this);
    }
  }

  void stopEventHandlerThread() { event_loop_start = false; }

  std::function<void(void)> getHandler() { return register_handler; }

  void setHandler(std::function<void(void)> reg_handler) {
    register_handler = reg_handler;
  }

  ~EventHandler() {
    if (reactor_thread.joinable()) {
      reactor_thread.join();
    }
  };

 private:
  // Don't expose the actual message parsing logic to the end user.
  void MessageLoop() {
    std::unique_lock<std::mutex> lock(msg_mutex);
    while (event_loop_start) {
      msg_cond.wait(lock, [&] { return !msg_queue.empty(); });
      auto msg = msg_queue.front();
      msg_queue.pop();
      ParseMessage(msg);
    }
  }

  void ParseMessage(const Message& msg) {
    // Assume that user ID is obtained from the first 2 bytes of the message
    // payload.
    int16_t user_id = (msg.data[1] << 8) | msg.data[0];

    if (msg.type == static_cast<int32_t>(MessageTypes::REGISTER_HANDLER)) {
      if (this->getHandler()) {
        RegisterEventHandler(user_id, msg.type, this->getHandler());
      } else {
        std::cerr << "Cannot register Event handler since it is NULL!"
                  << std::endl;
      }

    } else if (msg.type == static_cast<int32_t>(MessageTypes::REMOVE_HANDLER)) {
      RemoveEventHandler(user_id, msg.type);
    } else {
      const auto it = handler_map.find({user_id, msg.type});
      if (handler_map.end() != it) {
        std::function<void(void)> handler = it->second;

        // Call the function registered by a specific user, for a specific
        // message type.
        handler();

      } else {
        // Further analysis can be done on which key failed, exactly.
        std::cout << "One of the Keys " << user_id << " or " << msg.type
                  << " not found" << std::endl;
      }
    }
  }

  std::queue<Message> msg_queue;
  std::thread reactor_thread;
  std::function<void(void)> register_handler;
  std::atomic<bool> event_loop_start{false};
  std::map<std::pair<int16_t, int32_t>, std::function<void(void)>>
      handler_map;  // Create a std::map, containing user IDs and a
                    // map of message types and event handlers, for each user
                    // ID. User IDs can be set from outside.

  std::map<std::pair<int16_t, int32_t>, bool>
      register_status_map;  // Create a second std::map, containing
                            // user IDs and a map of message types and boolean
                            // flags, for each user ID. This map indicates
                            // whether a handler has completed its task or not,
                            // by making the handler set the flag on completion.

  std::mutex map_mutex;
  std::mutex msg_mutex;
  std::condition_variable msg_cond;
};
}  // namespace reactor

void TestRead1() { std::cout << "Reading from user1. " << std::endl; }

void TestWrite1() { std::cout << "Writing from user1. " << std::endl; }

void TestOpen1() { std::cout << "Opening a file from user1. " << std::endl; }

void TestRead2() { std::cout << "Reading from user2. " << std::endl; }

void TestWrite2() { std::cout << "Writing from user2. " << std::endl; }

void TestOpen2() { std::cout << "Opening a file from user2. " << std::endl; }

int main() {
  /* Simulating end users of a reactor. */

  reactor::Message message1 = {
      static_cast<int32_t>(reactor::MessageTypes::OPEN), {0, 1, 2, 3}};

  reactor::EventHandler event_handler(message1);
  event_handler.startEventHandlerThread();

  event_handler.RegisterEventHandler(
      256, static_cast<int32_t>(reactor::MessageTypes::READ), TestRead1);
  event_handler.RegisterEventHandler(
      256, static_cast<int32_t>(reactor::MessageTypes::OPEN), TestOpen1);
  event_handler.RegisterEventHandler(
      256, static_cast<int32_t>(reactor::MessageTypes::WRITE), TestWrite1);

  reactor::Message message2 = {
      static_cast<int32_t>(reactor::MessageTypes::READ), {1, 1, 2, 3}};
  event_handler.RegisterEventHandler(
      257, static_cast<int32_t>(reactor::MessageTypes::READ), TestRead2);
  event_handler.OnMessage(message2);

  reactor::Message msg_ptr = message2;

  int count = 0;
  while (1) {
    count++;
    std::cout << "count: " << count << std::endl;
    event_handler.OnMessage(std::move(msg_ptr));

    if (count % 2 == 0) {
      message1.type = count % 3;

      msg_ptr = message1;
      event_handler.RemoveEventHandler(
          ((message1.data[1] << 8) | message1.data[0]), message1.type);
    } else {
      message2.type = count % 7;

      msg_ptr = message2;
    }

    if (count >= 100 && count <= 200) {
      event_handler.stopEventHandlerThread();
    }

    if (count > 200) {
      event_handler.startEventHandlerThread();
    }

    if (count > 500) {
      event_handler.stopEventHandlerThread();
      break;
    }

    std::chrono::milliseconds change_interval(20);
    std::this_thread::sleep_for(change_interval);
  }
}