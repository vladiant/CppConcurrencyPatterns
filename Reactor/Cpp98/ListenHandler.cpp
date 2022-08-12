#include "ListenHandler.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "ClientHandler.h"
#include "Dispatcher.h"

ListenHandler::ListenHandler(Handler fd) : listenFd(fd) {}

ListenHandler::~ListenHandler() { close(listenFd); }

void ListenHandler::handleRead() {
  struct sockaddr_in peeraddr;
  socklen_t peerlen = sizeof(sockaddr_in);
  int connFd = ::accept(listenFd, (sockaddr*)&peeraddr, &peerlen);
  if (-1 == connFd) {
    return;
  }

  std::cout << "ip=" << inet_ntoa(peeraddr.sin_addr)
            << " port=" << ntohs(peeraddr.sin_port) << " fd:" << connFd
            << std::endl;

  EventHandler* h = new (std::nothrow) ClientHandler(connFd);
  assert(NULL != h);
  Dispatcher::getInstance()->registHandler(h);
}

void ListenHandler::handleWirte() {
  // nothing todo
}

void ListenHandler::handleError() {
  // nothing todo
}