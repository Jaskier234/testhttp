#include "http.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_MESSAGE_SIZE 256 // TODO sprawdzić zalecenia standardu
#define CRLF "\r\n"
#define SP " "
char HTTP[] = "http://";
#define HTTP_LEN 7
char HTTPS[] = "https://";
#define HTTPS_LEN 8
char METHOD[] = "GET";
#define METHOD_LEN 3
char HTTP_VERSION[] = "HTTP/1.1";
#define HTTP_VERSION_LEN 8
char HOST[] = "Host";
char CONNECTION[] = "Connection";
char CLOSE[] = "close";
char SET_COOKIE[] = "Set-Cookie"; // TODO sprawdzić literówki

int initialize_http_message(http_message *message) {
  message->message = malloc(sizeof(char) * INITIAL_MESSAGE_SIZE);
  if (message->message == NULL) {
    return -1;
  } 
  
  message->length = 0;
  message->capacity = INITIAL_MESSAGE_SIZE;

  return 0;
} 

int extend_message_capacity(http_message *message, size_t new_capacity) {
  if (new_capacity > message->capacity) { 
    new_capacity *= 2;

    void *new_message = realloc(message->message, new_capacity);
    if (new_message == NULL) {
      return -1;
    }

    message->message = new_message;
    message->capacity = new_capacity;
  }

  return 0;
}

int append(http_message *message, char * const string, size_t size) {
  // +4 for buffer
  if(extend_message_capacity(message, message->length + size + 4) != 0)
    return -1;

  memcpy(message->message + message->length, string, size);
  message->length += size;
  
  return 0;
}

int generate_request(http_message *message, char *target_url, char *cookie_file) {
  // parse target url
  if (memcmp(target_url, &HTTP, HTTP_LEN) == 0) {
    target_url += HTTP_LEN;
  } else if (memcmp(target_url, &HTTPS, HTTPS_LEN) == 0) {
    target_url += HTTPS_LEN;
  } else {
    return -1; // Incorrect url
  }

  char *host = target_url;
  target_url = strchr(target_url, '/');
  
  if (append(message, METHOD, strlen(METHOD)) != 0) return -1; 
  if (append(message, " ", 1) != 0) return -1; 
  if (append(message, target_url, strlen(target_url)) != 0) return -1; 
  if (append(message, " ", 1) != 0) return -1; 
  if (append(message, HTTP_VERSION, strlen(HTTP_VERSION)) != 0) return -1; 
  if (append(message, CRLF, strlen(CRLF)) != 0) return -1; 

  *target_url = 0; // set first byte of target url to 0 so that host contains correct address.

  if (add_header(message, (char*)&HOST, host) != 0) {
    return -1;
  }

  // Set cookies
  FILE *cookies = fopen(cookie_file, "r");
  if (cookies == NULL) {
    return -1; // cookie file does not exist
  }

  char *cookie_buffer = NULL;
  size_t cookie_buffer_len = 0;
  int res;

  while ((res = getline(&cookie_buffer, &cookie_buffer_len, cookies)) != -1) {
    size_t cookie_len = strlen(cookie_buffer);
    printf("cookie len: %ld buffer len: %ld\n", cookie_len, cookie_buffer_len);

    if (add_header(message, (char*)&SET_COOKIE, cookie_buffer) != 0) {
      return -1;
    }
  }
  free(cookie_buffer);

  // Set connection close 
  if (add_header(message, (char*)&CONNECTION, (char*)&CLOSE) != 0) {
    return -1;
  }

  size_t cookie_len = strlen(cookie_buffer);
  printf("cookie len: %ld buffer len: %ld\n", cookie_len, cookie_buffer_len);

  if (append(message, CRLF, strlen(CRLF)) != 0) return -1;

  return 0;
}

int add_header(http_message *message, char *header_name, char *value) {
  if (append(message, header_name, strlen(header_name)) != 0) return -1;
  if (append(message, ":", 1) != 0) return -1;
  if (append(message, value, strlen(value)) != 0) return -1;
  if (append(message, CRLF, strlen(CRLF)) != 0) return -1;

  return 0;
}

// TODO czy ta funkcja potrzebna?
void end_message(http_message *message) {
}
