#include <stdio.h>
#include <string.h>
#include "connect.h"
#include "http.h"

#define N_c 9
#define P_c 1
char test_connection[N_c + P_c][30] = {
  // negative
  "incorrect",
  "1234",
  "1270013030",
  "127.0.0.1.3030",
  "",
  "127.incorrect.0.0.1:3030",
  ":",
  // TODO poprawny adres, ale nie ma pod nim serwera "111.121.34.90:3030",
  ":3030",
  "127.0.0.1:",
  // positive
  "127.0.0.1:3030"
  // TODO połączenie ze studentsem
  // TODO połaczenie z maszyną wirtualną
};

#define URL_TESTS_NEG 5
char test_url[URL_TESTS_NEG][30] = {
  "site/file",
  "ttp://file",
  "http//site/file/fle/fiel",
  "",
  "http://site"
};

void make_connection_test();
void add_status_line_test();

int main(int argc, char *argv[]) {
  make_connection_test();
  add_status_line_test();

  printf("Tests finished\n");

  return 0;
}

void make_connection_test() {
  // incorrect string format 

  int failed = 0;

  for (int i=0; i < N_c; i++) {
    if (make_connection(test_connection[i]) != -1) {
      printf("make_test_connection() test %d failed\n", i);
      failed = 1;
    }
  }
  // correct adderss
  for (int i = N_c; i < N_c + P_c; i++) {
    if (make_connection(test_connection[i]) == -1) {
      printf("make_test_connection() test %d failed\n", i);
      failed = 1;
    }
  }

  if (!failed) printf("make_connection() passed\n");
} 

void add_status_line_test() {
  http_message message1;

  int failed = 0;

  for (int i = 0; i < URL_TESTS_NEG; i++) {
    if (add_status_line(NULL, test_url[i]) == 0) {
      printf("make_test_connection() test %d failed\n", i);
      failed = 1;
    }
  }
  
  char long_url[10040] = "https://";

  memset(long_url + 8, 'a', 9000);

  long_url[20] = '/';

  if (add_status_line(&message1, long_url) != 0) {
    printf("add_status_line long_url test failed\n");
    failed = 1;
  }

  // TODO:
  // -incorrect addresses
  // -long adderss
  // -correct addresses
  
  if (!failed) printf("add_status_line_test() passed\n");
}
