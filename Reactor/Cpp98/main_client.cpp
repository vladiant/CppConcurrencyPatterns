#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const char DATA[] = "Test message";

/*
 * This program creates a socket and initiates a connection with
 * the socket given in the command line. Some data are sent over the
 * connection and then the socket is closed, ending the connection.
 */
int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in server;
  char buf[1024];

  /* Create socket. */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("opening stream socket");
    return EXIT_FAILURE;
  }

  struct hostent *hp;
  hp = gethostbyname("localhost");
  memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

  server.sin_family = AF_INET;
  server.sin_port = htons(5188);  // Same as server

  if (connect(sock, (struct sockaddr *)&server, sizeof server) == -1) {
    perror("connecting stream socket");
    return EXIT_FAILURE;
  }

  if (write(sock, DATA, sizeof DATA) == -1) perror("writing on stream socket");
  close(sock);

  return EXIT_SUCCESS;
}
