/* Set of functions for constructing and parsing http messages */

#include <stddef.h>

// represents http message
typedef struct {
  char *message;
  size_t length; // length of the message
  size_t capacity; // size of allocated memory
} http_message;

// initializes http message
int initialize_http_message(http_message*);

// Adds header to http messsage
// returns 0 on success or -1 if error occurs.
int add_header(http_message* message, char* header, char *value);

// Adds CRLF at the end of the message
void end_message(http_message *message);
