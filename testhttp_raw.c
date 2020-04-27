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

  parsed_http_response response = parse_message(socket);

  if (response.failed) {
    fatal("Incorrect http response");
  }
  
  if (response.status_code != 200) {
    printf("%s\n", response.status_line);
    return 0;
  }
  
  printf("%s", response.cookies.message);
  printf("Dlugosc zasobu: %ld\n", response.real_body_length);

  return 0;
}
