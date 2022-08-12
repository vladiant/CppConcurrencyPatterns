#include "EpollPoller.h"

#include <assert.h>
#include <errno.h>

#include <iostream>

typedef std::vector<struct epoll_event> EventList;
epollPoller::epollPoller() : _events(16), epollFd(::epoll_create(100000)) {}

epollPoller::~epollPoller() { close(epollFd); }

int epollPoller::waitEvent(std::map<Handler, EventHandler*>& handlers,
                           int timeout) {
  std::cout << "start wait" << std::endl;
  int nready =
      epoll_wait(epollFd, &_events[0], static_cast<int>(_events.size()), -1);
  if (-1 == nready) {
    std::cout << "WARNING: epoll_wait error " << errno << std::endl;
    return -1;
  }

  for (int i = 0; i < nready; i++) {
    Handler handle = _events[i].data.fd;
    if ((EPOLLHUP | EPOLLERR) & _events[i].events) {
      assert(handlers[handle] != NULL);
      (handlers[handle])->handleError();
    } else {
      if ((EPOLLIN)&_events[i].events) {
        assert(handlers[handle] != NULL);
        (handlers[handle])->handleRead();
      }

      if (EPOLLOUT & _events[i].events) {
        (handlers[handle])->handleWirte();
      }
    }
  }
  if (static_cast<size_t>(nready) == _events.size()) {
    _events.resize(_events.size() * 2);
  }
  return nready;
}

int epollPoller::regist(Handler handler) {
  struct epoll_event ev;
  ev.data.fd = handler;
  ev.events |= EPOLLIN;
  ev.events |= EPOLLET;

  if (epoll_ctl(epollFd, EPOLL_CTL_ADD, handler, &ev) != 0) {
    if (errno == ENOENT) {
      if (epoll_ctl(epollFd, EPOLL_CTL_ADD, handler, &ev) != 0) {
        std::cout << "epoll_ctl add error " << errno << std::endl;
        return -1;
      }
    }
  }
  return 0;
}

int epollPoller::remove(Handler handler) {
  struct epoll_event ev;
  if (epoll_ctl(epollFd, EPOLL_CTL_DEL, handler, &ev) != 0) {
    std::cout << "epoll_ctl del error" << errno << std::endl;
    return -1;
  }
  return 0;
}