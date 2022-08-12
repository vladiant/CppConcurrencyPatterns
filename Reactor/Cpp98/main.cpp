#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <iostream>

#include "Dispatcher.h"
#include "ListenHandler.h"

#define ERR_EXIT(m)     \
  do {                  \
    perror(m);          \
    exit(EXIT_FAILURE); \
  } while (0)

int main() {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) ERR_EXIT("socket error");

  int fd_flags = ::fcntl(listenfd, F_GETFL, 0);
  fd_flags |= FD_CLOEXEC;
  fd_flags |= O_NONBLOCK;
  fcntl(listenfd, F_SETFD, fd_flags);

  struct sockaddr_in seraddr;
  seraddr.sin_family = AF_INET;
  seraddr.sin_port = htons(5188);  // Same as client
  seraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    ERR_EXIT("setsockopt");

  if (bind(listenfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) < 0)
    ERR_EXIT("bind");

  if (listen(listenfd, 5) < 0) ERR_EXIT("listen");

  EventHandler* handler = new ListenHandler(listenfd);
  Dispatcher::getInstance()->registHandler(handler);

  while (true) {
    Dispatcher::getInstance()->dispatchEvent(1000);
  }

  return 0;
}
