#include "connect.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "err.h"

int make_connection(char *address_string) {
  // Check ip string
  char *colon = strchr(address_string, ':');
  if (colon == NULL) {
    return -1; // address_string is incorrect
  }

  char *ip = address_string;
  *colon = 0; // split address string into two parts
  colon++;
  char *port = colon;

  int sock;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  // 'converting' host/port in string to struct addrinfo
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  int err = getaddrinfo(ip, port, &addr_hints, &addr_result);
  if (err == EAI_SYSTEM) { // system error
//    syserr("getaddrinfo: %s", gai_strerror(err));
    return -1;
  }
  else if (err != 0) { // other error (host not found, etc.)
//    fatal("getaddrinfo: %s", gai_strerror(err));
    return -1;
  }

  // initialize socket according to getaddrinfo results
  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if (sock < 0)
    return -1;

  // connect socket to the server
  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
    return -1;

  freeaddrinfo(addr_result);
  
  return sock;
}

