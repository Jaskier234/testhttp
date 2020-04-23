#include "http.h"
#include <string.h>
#include <stdlib.h>

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

int add_status_line_and_host(http_message *message, char *target_url) {
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
  size_t target_url_len = strlen(target_url);

  size_t status_line_len = METHOD_LEN  + 1 + target_url_len + 1 + HTTP_VERSION_LEN; // +1 for whitespaces

  if(extend_message_capacity(message, status_line_len + 4) != 0) // +4 for buffer
    return -1;

  strcpy(message->message + message->length, METHOD);
  message->length += METHOD_LEN;

  strcpy(message->message + message->length, " ");
  message->length += 1;

  strcpy(message->message + message->length, target_url);
  message->length += target_url_len;

  strcpy(message->message + message->length, " ");
  message->length += 1;

  strcpy(message->message + message->length, HTTP_VERSION);
  message->length += HTTP_VERSION_LEN;

  strcpy(message->message + message->length, CRLF);
  message->length += 2;

  *target_url = 0; // set first byte of target url to 0 so that host contains correct address.

  return add_header(message, (char*)&HOST, host);
}

int add_header(http_message *message, char *header_name, char *value) {
  int header_name_len = strlen(header_name);
  int value_len = strlen(value);
  // one char for ":", two for CRLF, one for NULL at the end and 4 more as a buffer
  int header_len = header_name_len + value_len + 8;

  if(extend_message_capacity(message, header_len) != 0) 
    return -1;

  strcpy(message->message + message->length, header_name);
  message->length += header_name_len;

  strcpy(message->message + message->length, ":");
  message->length += 1;
  
  strcpy(message->message + message->length, value);
  message->length += value_len;

  strcpy(message->message + message->length, CRLF);
  message->length += 2;

  return 0;
}

void end_message(http_message *message) {
  strcpy(message->message + message->length, CRLF);
  message->length += 2;
}
