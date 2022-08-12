#ifndef DISPATHER_H_H_
#define DISPATHER_H_H_
#include <map>

#include "EventHandler.h"
#include "Poller.h"

class Dispatcher {
 public:
  static Dispatcher* getInstance() { return instance; }
  int registHandler(EventHandler* handler);
  void removeHander(EventHandler* handler);
  void dispatchEvent(int timeout = 0);

 private:
  //单例模式
  Dispatcher();
  ~Dispatcher();
  //只声明不实现,用于禁止拷贝构造和赋值构造
  Dispatcher(const Dispatcher&);
  Dispatcher& operator=(const Dispatcher&);

 private:
  Poller* _poller;
  std::map<Handler, EventHandler*> _handlers;

 private:
  static Dispatcher* instance;
};

#endif  // !DISPATHER_H_H_