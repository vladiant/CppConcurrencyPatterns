#ifndef LISTEN_HANDLER_H_
#define LISTEN_HANDLER_H_

#include "EventHandler.h"

class ListenHandler : public EventHandler {
 public:
  ListenHandler(Handler fd);
  ~ListenHandler();
  Handler getHandler() { return listenFd; }
  void handleRead();
  void handleWirte();
  void handleError();

 private:
  Handler listenFd;
};

#endif  // !LISTEN_HANDLER_H_