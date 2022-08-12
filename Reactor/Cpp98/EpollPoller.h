#ifndef EPOLL_POLLER_H_
#define EPOLL_POLLER_H_
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

#include "Poller.h"

class epollPoller : public Poller {
 public:
  epollPoller();
  ~epollPoller();
  int waitEvent(std::map<Handler, EventHandler*>& handlers, int timeout = 0);
  int regist(Handler handler);
  int remove(Handler handler);

 private:
  typedef std::vector<struct epoll_event> EventList;
  EventList _events;
  int epollFd;
};

#endif  // !EPOLL_POLLER_H_