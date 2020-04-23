/* Set of functions for constructing and parsing http messages */

#include <stddef.h>

// Represents http message
typedef struct {
  char *message;
  size_t length; // length of the message
  size_t capacity; // size of allocated memory
} http_message;

// Initialize http message
int initialize_http_message(http_message*);

// If message->capacity is smaller than new_size function tries to 
// increase it. Otherwise it does nothing.
// Returns -1 if realloc failed. If 0 was returned, message capacity
// is at least new_size.
int extend_message_capacity(http_message *message, size_t new_capacity);

// Add request status line. 
// Second parameter is full URL of target site.
// Returns 0 on success and -1 on failure.
int add_status_line(http_message *message, char *target_url);

// Add header to http messsage
// returns 0 on success or -1 if error occurs.
int add_header(http_message* message, char* header, char *value);

// Add CRLF at the end of the message
void end_message(http_message *message);
