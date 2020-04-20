#include <stdio.h>
#include "connect.h"

#define N 9
#define P 1
char test_string[N + P][30] = {
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

void make_connection_test();

int main(int argc, char *argv[]) {
  make_connection_test();

  printf("Tests finished\n");

  return 0;
}

void make_connection_test() {
  // incorrect string format 

  int failed = 0;

  for (int i=0; i < N; i++) {
    if (make_connection(test_string[i]) != -1) {
      printf("make_test_connection() test %d failed\n", i);
      failed = 1;
    }
  }
  // correct adderss
  for (int i = N; i < N + P; i++) {
    if (make_connection(test_string[i]) == -1) {
      printf("make_test_connection() test %d failed\n", i);
      failed = 1;
    }
  }

  if (!failed) printf("make_connection() passed\n");
} 
