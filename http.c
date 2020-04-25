#include "http.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define INITIAL_MESSAGE_SIZE 256
#define CRLF "\r\n"
#define SP ' '
#define HTAB 9
char HTTP[] = "http://";
#define HTTP_LEN 7
char HTTPS[] = "https://";
#define HTTPS_LEN 8
char METHOD[] = "GET";
#define METHOD_LEN 3
char HTTP_VERSION[] = "HTTP/1.1";
#define HTTP_VERSION_LEN 8 // TODO może użyć strlen
char HOST[] = "Host";
char CONNECTION[] = "Connection";
char CLOSE[] = "close";
char COOKIE[] = "Cookie";
char COOKIE_VERSION[] = "$Version=0;";
char SET_COOKIE[] = "Set-Cookie"; // TODO sprawdzić literówki
char TRANSFER_ENCODING[] = "Transfer-Encoding";
char CONTENT_LENGTH[] = "Content-Length";
char CHUNKED[] = "chunked"; // TODO make this case insensitive

int initialize_http_message(http_message *message) {
  message->message = malloc(sizeof(char) * INITIAL_MESSAGE_SIZE);
  if (message->message == NULL) {
    return -1;
  } 
  
  message->length = 0;
  message->capacity = INITIAL_MESSAGE_SIZE;
  memset(message->message, 0, message->capacity);

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
  
  http_message cookie_value;
  initialize_http_message(&cookie_value);

  while ((res = getline(&cookie_buffer, &cookie_buffer_len, cookies)) != -1) {
    char *newline = strchr(cookie_buffer, '\n');
    if (newline != NULL) {
      *newline = 0;
    }
    cookie_value.length = 0;

    if (append(&cookie_value, (char*)&COOKIE_VERSION, strlen((char*)&COOKIE_VERSION)) != 0) return -1; 
    if (append(&cookie_value, cookie_buffer, strlen(cookie_buffer)) != 0) return -1; 

    if (add_header(message, (char*)&COOKIE, cookie_value.message) != 0) {
      return -1;
    }
  }
  free(cookie_buffer);

  // Content-length

  // Set connection close 
  if (add_header(message, (char*)&CONNECTION, (char*)&CLOSE) != 0) {
    return -1;
  }

  // Endind empty line
  if (append(message, CRLF, strlen(CRLF)) != 0) return -1;

  return 0;
}

int add_header(http_message *message, char *header_name, char *value) {
  if (append(message, header_name, strlen(header_name)) != 0) return -1;
  if (append(message, ":", 1) != 0) return -1;
  if (append(message, value, strlen(value)) != 0) return -1;
  if (append(message, CRLF, strlen(CRLF)) != 0) return -1;

  // Użyć sprintf

  return 0;
}

// Read header line until CRLF
// Buffer should be freed afterwards by caller
// Returns -1 on failrue or length of header on success
int get_header(char **buffer, FILE *file) {
  http_message header;
  initialize_http_message(&header);
  
  int res = 0;
  char *part_buffer = NULL;
  size_t part_buffer_len = 0;
    
  do {
    res = getline(&part_buffer, &part_buffer_len, file);
    if (res == -1) {
      // TODO free
      return -1;
    }
     
    append(&header, part_buffer, res);
  } while (memcmp((header.message + header.length - 2), CRLF, 3) != 0);
  
  free(part_buffer);
  *buffer = header.message;
  return header.length;
}

parsed_http_response parse_message(int fd) {
  // TODO cleaner way to initialize
  parsed_http_response response;
  initialize_http_message(&response.cookies);
  response.content_length = -1;
  response.real_body_length = 0;
  response.transfer_encoding = -1; // -1 none. 1 chunked. 0 other
  response.failed = 1; // Failed initially is set to true so response is considered
                       // incorrect until the end of the function.

  FILE *http_response = fdopen(fd, "r");
  if (http_response == NULL) {
    return response;
  }

  // Parse headers
  char *current_line = NULL;
  size_t current_line_len = 0;
  char *next_line = NULL;
  size_t next_line_len = get_header(&next_line, http_response);

  if (next_line_len == -1) {
    return response;  
  }

  int status_line = 1;

  do {
    free(current_line);
    current_line = next_line;
    current_line_len = next_line_len;
    next_line = NULL;
    next_line_len = get_header(&next_line, http_response);

    if (next_line_len == -1) {
      return response;  
    }

    // Process current line
    // TODO obs-fold: zrobić tak, żeby w next-line znajdował się cały header

    // Remove CRLF from the end
    *(current_line + current_line_len - 1) = 0;
    *(current_line + current_line_len - 2) = 0;
    current_line_len -= 2;
  
    if (status_line) { // Parse status line
      // Check http version
      if (memcmp(current_line, HTTP_VERSION, HTTP_VERSION_LEN) != 0) {
        return response;
      }

      if (current_line[8] != SP) return response;

      // Check if status code is correct
      if (current_line[9] > '9' || current_line[9] < '0') return response;
      if (current_line[10] > '9' || current_line[10] < '0') return response;
      if (current_line[11] > '9' || current_line[11] < '0') return response;

      if (current_line[12] != SP) return response;

      char *status_code = current_line + 9; 

      response.status_code = strtol(status_code, NULL, 10);
      if (response.status_code < 100 || response.status_code >= 1000) return response;

      response.status_line = current_line;

      if (response.status_code != 200) {
        response.failed = 0;
        return response;
      }

      status_line = 0;
    } else { // Parse headers
      // Split header into name and value
      char *field_name = current_line;

      char *colon_pos = memchr(current_line, ':', current_line_len); 
      if (colon_pos == NULL) {
        return response;
      }
      char *field_value = colon_pos + 1;

      size_t field_name_len = field_value - field_name - 1;
      size_t field_value_len = current_line_len - field_name_len - 1;
      
      // skip trailing whitespaces
      char *it = field_value + field_value_len - 1;
      while (it >= field_value && (*it == SP || *it == HTAB)) {
        *it = 0;
        it--;
        field_value_len--;
      }

      // skip leading whitespaces
      while (*field_value == SP || *field_value == HTAB) {
         field_value_len--; 
         field_value++;
      }
      
      // check field name
      if (memcmp(CONTENT_LENGTH, field_name, field_name_len) == 0) {
        // parse content-length
        if (*field_value == '"')  {
          if (*(field_value + field_value_len - 1) == '"') {
            *(field_value + field_value_len - 1) = 0;
            field_value++;
            field_value_len -= 2;
          } else {
            // incorrect quoting
            return response;
          }
        }

        char *endptr = NULL;
        int content_length = strtol(field_value, &endptr, 10);
        if ((content_length == 0 && *endptr != 0) 
            || content_length == LONG_MAX 
            || content_length == LONG_MIN) {
          return response;
        }

        if (response.content_length == -1) {
          response.content_length = content_length;
        } else if (response.content_length != content_length) {
          response.content_length = -2; // Content-Length field is incorrect
        }
      } else if (memcmp(TRANSFER_ENCODING, field_name, field_name_len) == 0) {
        if (response.transfer_encoding == -1) {
          response.transfer_encoding = 0;
        }

        int cmp_res;
        while ((cmp_res = memcmp(field_value, CHUNKED, strlen(CHUNKED))) != 0) {
          // find delimiter (',')
          char *delim = memchr(field_value, ',', field_value_len);
          if (delim == NULL) {
            break;
          }
          // move field_value beggining one char after delimiter
          delim++;
          field_value_len -= delim - field_value;
          field_value = delim;

          // skip whitespaces after delimiter
          while(*field_value == SP || *field_value == HTAB) {
            field_value++;
            field_value_len--;
          }
        } 

        if (cmp_res == 0) {
          response.transfer_encoding = 1; // transfer-encoding: chunked
        }
  
      } else if (memcmp(SET_COOKIE, field_name, field_name_len) == 0) {
        char *delim = memchr(field_value, ';', field_value_len);
        if (delim != NULL) {
          *delim = 0;
          field_value_len = delim - field_value;
        }
        if (append(&response.cookies, field_value, field_value_len) != 0) return response;
        if (append(&response.cookies, "\n", 1) != 0) return response;
      } else {
        // dont care
      }
    }

  } while (memcmp(CRLF, next_line, 3) != 0);

  if (fclose(http_response) != 0) {
    return response;  
  }

  if (response.transfer_encoding == 1) {
    // Chunked
  } else if (response.transfer_encoding == 0) {
    // other encoding. Read till close
  } else if (response.content_length != -1) {
    // correct content length. Read it
    response.real_body_length = response.content_length;
  } else {
    // none of above. Read till close
  }

  response.failed = 0;
  return response;
}

// TODO czy ta funkcja potrzebna?
void end_message(http_message *message) {
}
