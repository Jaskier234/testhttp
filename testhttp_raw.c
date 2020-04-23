#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "connect.h"
#include "http.h"
#include "err.h"

int main(int argc, char *argv[]) {

  if (argc != 4) {
    fatal("Usage: %s <address>:<port> <cookie file> <tested address> ...\n", argv[0]);
  }

  http_message request;

  initialize_http_message(&request);

  if (-1 == generate_request(&request, argv[3], argv[2])) {
    fatal("generate request failed");
  }

  int socket = make_connection(argv[1]);

  write(socket, request.message, request.length);

  char *buffer = malloc(sizeof(char) * 1000);

  // TODO read in loop
  read(socket, buffer, 1000);

  printf("%s", buffer);

  return 0;
}
