#include "ClientHandler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

ClientHandler::ClientHandler(Handler fd) : clientFd(fd) {
  memset(revBuff, 0, sizeof(revBuff));
}

ClientHandler::~ClientHandler() { close(clientFd); }

void ClientHandler::handleRead() {
  if (read(clientFd, revBuff, sizeof(revBuff))) {
    std::cout << "recv client:" << clientFd << ":" << revBuff << std::endl;
    write(clientFd, revBuff, strlen(revBuff));
    memset(revBuff, 0, sizeof(revBuff));
  } else {
    std::cout << "client close fd:" << clientFd << std::endl;
    close(clientFd);
  }
}

void ClientHandler::handleWirte() {
  // nothing todo
}

void ClientHandler::handleError() {
  // nothing todo
  std::cout << "client close:" << clientFd << std::endl;
}