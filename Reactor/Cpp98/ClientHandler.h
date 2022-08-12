#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_

#include "EventHandler.h"

class ClientHandler : public EventHandler {
 public:
  ClientHandler(Handler fd);
  ~ClientHandler();
  Handler getHandler() { return clientFd; }
  virtual void handleRead();
  virtual void handleWirte();
  virtual void handleError();

 private:
  Handler clientFd;
  char revBuff[1024];
};

#endif  // !CLIENT_HANDLER_H_