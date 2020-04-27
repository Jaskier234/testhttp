#include "http.h"
#include "err.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#define INITIAL_MESSAGE_SIZE 256
#define BUFFER_SIZE (1<<13) // ~8kb
#define CRLF "\r\n"
#define CR '\r'
#define LF '\n'
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
char SET_COOKIE[] = "set-cookie"; // TODO sprawdzić literówki
char TRANSFER_ENCODING[] = "transfer-encoding";
char CONTENT_LENGTH[] = "content-length";
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
  if (extend_message_capacity(message, message->length + size + 4) != 0)
    return -1;

  memcpy(message->message + message->length, string, size);
  message->length += size;
  *(message->message + message->length) = 0; // Make sure that message->message is null-terminated
  
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

  if (target_url == NULL) {
    // TODO
  }
  
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
      *newline = 0; // Remove new line character
    }

    if (append(&cookie_value, cookie_buffer, strlen(cookie_buffer)) != 0) return -1; 
    if (append(&cookie_value, ";", 1) != 0) return -1; 
  }

  if (cookie_value.length > 0) { // Cookie file is non-empty
    if (add_header(message, (char*)&COOKIE, cookie_value.message) != 0) {
      return -1;
    }
  }

  free(cookie_buffer);
  free(cookie_value.message);

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

// Returns pointer to first LF character which is preceded by CR character 
// or NULL if there is no such character inside limit.
// prev_char is character at index -1. So if string[0] == LF and prev_char == CR.
// string is returned.
char *seek_crlf(char *string, size_t limit, char prev_char) {
  char *begin = string;

  if (*string == LF && prev_char == CR)
    return string;

  string++;
  for (; string - begin < limit; string++) {
    if (*string == LF && *(string - 1) == CR)
      return string;
  }

  return NULL;
}

// Read from the socket until CRLF into allocated buffer and save this buffer 
// into *line. If line is NULL data is ignored. If this function is called with
// different file pointer before end of current file error is returned.
// Returns -1 on error or length of line otherwise.
int get_line(char **line, FILE *file) { 
  static char *buffer = NULL;
  static char *buff_ptr = NULL; // Points to first non-processed byte 
  static FILE *current_file = NULL;
  char last_char; // last character in previous buffer

  if (buffer == NULL) { // new FILE*
    buffer = malloc(sizeof(char) * BUFFER_SIZE);
    current_file = file;
    buff_ptr = buffer + BUFFER_SIZE;
  }

  if (current_file != file) // get_line called with new file before eof
    return -1;
    
  http_message result;
  initialize_http_message(&result);

  int crlf = 0;
  int eof = 0;
  int buffer_size = BUFFER_SIZE; // Most of the times buffer_size == BUFFER_SIZE, 
                                 // but at the end of file buffer_size is smaller

  do { 
    // Character before buff_ptr. If buff_ptr == buffer this is last character
    // in previous buffer
    char prev_char = ((buff_ptr == buffer)?last_char:*(buff_ptr - 1)); 
    // search from buff_ptr to the end of buffer
    char *newline = seek_crlf(buff_ptr, buffer_size - (buff_ptr - buffer), prev_char); 

    if (newline != NULL) { 
      crlf = 1; // CRLF found
      newline++; // Put newline pointer after LF character
    } else {
      newline = buffer + buffer_size;
    }

    // append current buffer to result.
    if (append(&result, buff_ptr, newline - buff_ptr) == -1) return -1;

    buff_ptr = newline;

    if (crlf == 0 && eof == 0) { // No new line found. Have to read more data
      
      last_char = *(buffer + buffer_size - 1);

      // read new buffer
      int res = fread(buffer, sizeof(char), BUFFER_SIZE, current_file);
      buff_ptr = buffer; // reset buff_ptr to the beginning of the buffer
      if (res < BUFFER_SIZE) {
        if (feof(current_file)) {
          eof = 1;
        } else {
          return -1; // error in fread occured
        }
      }
    }
  } while (crlf == 0);

  if (eof) {
    // TODO reset
  }

  *line = result.message;

  return result.length;
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

// Read chunked message body. Returns message length or -1 if error occured
size_t read_chunked(FILE *conn) {
  char *chunk_size_buff = NULL;
  size_t chunk_size_buff_len = 0;
  size_t chunk_size = 0;
  size_t total_size = 0;
  char *chunk_data = NULL;
  size_t chunk_data_len = 0;

  char *buffer = malloc(sizeof(char) * BUFFER_SIZE);
  
  int res;

  do {
    res = getline(&chunk_size_buff, &chunk_size_buff_len, conn);
    if (res == -1) return -1;
    
    chunk_size = strtol(chunk_size_buff, NULL, 16);
    total_size += chunk_size;

    if (chunk_size == 0) {
      break;
    }

    // Read chunk data
    size_t data_to_read = chunk_size + 2; // chunk data + CRLF
    while (data_to_read > 0) {
      int nitems = (BUFFER_SIZE <= data_to_read)?BUFFER_SIZE:data_to_read;
      res = fread(buffer, sizeof(char), nitems, conn);
     
      if (res == 0 && ferror(conn))
        return -1; // error occured

      data_to_read -= res;
    }
  } while (chunk_size > 0);

  free(chunk_size_buff);
  free(chunk_data);
  free(buffer);

  return total_size;
}

void strtolower(char *string) {
  while (*string != 0) {
    *string = tolower(*string);
    string++;
  }
}

parsed_http_response parse_message(int fd) {
  // TODO cleaner way to initialize
  parsed_http_response response;
  initialize_http_message(&response.cookies);
  response.content_length = -1;
  response.real_body_length = 0;
  response.chunked = 0; // -1 none. 1 chunked. 0 other
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

      *colon_pos = 0; // Split name and value
      
      strtolower(field_name);

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
        if (memcmp(field_value, CHUNKED, strlen(CHUNKED)) != 0) {
          response.chunked = 1;
        } else {
          // Unsupported transfer encoding.
          fatal("Unsupported transfer encoding");
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

  if (response.chunked == 1) {
    // Chunked
    response.real_body_length = read_chunked(http_response);

  } else {
    // Read till close

    char *buffer = malloc(sizeof(char) * BUFFER_SIZE);

    int res;

    do {
      res = fread(buffer, sizeof(char), BUFFER_SIZE, http_response);
      response.real_body_length += res;
    } while (res == BUFFER_SIZE);

// TODO    if (/*check stream error*/) {}
  }

  if (fclose(http_response) != 0) {
    return response;  
  }

  response.failed = 0;
  return response;
}

// TODO czy ta funkcja potrzebna?
void end_message(http_message *message) {
}
