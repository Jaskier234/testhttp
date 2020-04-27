/* Set of functions for constructing and parsing http messages */

#include <stddef.h>
#include <stdio.h>

// Represents http message
typedef struct {
  char *message;
  size_t length; // length of the message
  size_t capacity; // size of allocated memory
} http_message;

typedef struct {
  http_message cookies;
  int content_length;
  size_t real_body_length;
  int chunked;
  int status_code;
  char *status_line;
  int failed;
} parsed_http_response;

// Initialize http message
int initialize_http_message(http_message*);

// If message->capacity is smaller than new_size function tries to 
// increase it. Otherwise it does nothing.
// Returns -1 if realloc failed. If 0 was returned, message capacity
// is at least new_size.
int extend_message_capacity(http_message *message, size_t new_capacity);

// Appends given string to message.
int append(http_message *message, char * const string, size_t size);

// Add request status line. 
// Second parameter is full URL of target site.
// Returns 0 on success and -1 on failure.
int generate_request(http_message *message, char *target_url, char *cookie_file);

// Add header to http messsage
// returns 0 on success or -1 if error occurs.
int add_header(http_message* message, char* header, char *value);

// Parse http response.
parsed_http_response parse_message(FILE *conn);
