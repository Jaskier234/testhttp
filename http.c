#include "http.h"

#define INITIAL_MESSAGE_SIZE 256 // TODO sprawdzić zalecenia standardu
#define CRLF "\r\n"

int initialize_http_message(http_message *message) {
  message->message = malloc(sizeof(char) * INITIAL_MESSAGE_SIZE);
  if (message->message == NULL) {
    return -1;
  } 
  
  message->length = 0;
  message->capacity = INITIAL_MESSAGE_SIZE;

  return 0;
} 

int add_header(http_message *message, char *header_name, char *value) {
  int header_name_len = strlen(header_name);
  int value_len = strlen(value);
  // one char for ":", two for CRLF, one for NULL at the end and 4 more so that
  // realloc in end_message is not needed.
  int header_len = header_name_len + value_len + 8;

  // realloc if capacity is too small
  if (message->length + header_len > message->capacity) { 
    int new_size = message->capacity * 2;
    if (new_size < message->length + header_len) { // in case capacity*2 is to small
      new_size = message->length + header_len;
    }

    message->message = realloc(message->message, new_size);
    if (message->message == NULL) {
      return -1;
    }

    message->capacity = new_size;
  }

  strcpy(message->message + message->length, header_name);
  message->length + header_name_len;

  strcpy(message->message + message->length, ":");
  message->length + 1;
  
  strcpy(message->message + message->length, value);
  message->length + value_len;

  strcpy(message->message + message->length, CRLF);
  message->length + 2;

  return 0;
}

void end_message(http_message *message) {
  strcpy(message->message + message->length, CRLF);
  message->length + 2;
}
