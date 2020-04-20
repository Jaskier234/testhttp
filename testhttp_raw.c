#include <stdio.h>
#include "connect.h"
#include "err.h"

int main(int argc, char *argv[]) {

  if (argc < 3) {
    fatal("Usage: %s host port message ...\n", argv[0]);
  }

  return 0;
}
