#ifndef EVENT_HANDLER_H_
#define EVENT_HANDLER_H_

typedef int Handler;

class EventHandler {
 public:
  EventHandler() {}
  virtual ~EventHandler() {}
  virtual Handler getHandler() = 0;
  virtual void handleRead() = 0;
  virtual void handleWirte() = 0;
  virtual void handleError() = 0;
};

#endif  // !EVENT_HANDLER_H_