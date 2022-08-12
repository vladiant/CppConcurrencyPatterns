#include "Dispatcher.h"

#include <assert.h>

#include "EpollPoller.h"

Dispatcher* Dispatcher::instance = new Dispatcher();

Dispatcher::Dispatcher() {
  _poller = new (std::nothrow) epollPoller();
  assert(NULL != _poller);
}

Dispatcher::~Dispatcher() {
  std::map<Handler, EventHandler*>::iterator iter = _handlers.begin();
  for (; iter != _handlers.end(); ++iter) {
    delete iter->second;
  }

  if (_poller) delete _poller;
}

int Dispatcher::registHandler(EventHandler* handler) {
  Handler h = handler->getHandler();
  if (_handlers.end() == _handlers.find(h)) {
    _handlers.insert(std::make_pair(h, handler));
  }
  return _poller->regist(h);
}

void Dispatcher::removeHander(EventHandler* handler) {
  Handler h = handler->getHandler();
  std::map<Handler, EventHandler*>::iterator iter = _handlers.find(h);
  if (iter != _handlers.end()) {
    delete iter->second;
    _handlers.erase(iter);
  }
  _poller->remove(h);
}

void Dispatcher::dispatchEvent(int timeout) {
  _poller->waitEvent(_handlers, timeout);
}