#ifndef POLLER_H_H_
#define POLLER_H_H_
#include <map>

#include "EventHandler.h"

class Poller {
 public:
  Poller() {}
  virtual ~Poller() {}
  virtual int waitEvent(std::map<Handler, EventHandler*>& handlers,
                        int timeout = 0) = 0;
  virtual int regist(Handler handler) = 0;
  virtual int remove(Handler handler) = 0;

 private:
};

#endif  // !POLLER_H_H_