#include "octet.h"

#include <assert.h>
#include <stdio.h>

void mark_no_roots() { printf(" ======GC====== "); }

void AllocAndPrintTest() {
  odump();
  ogc();
  odump();
  for (byte j = 0; j < 10; j++) {
    printf("Round %d:\n", j);
    for (byte i = 1; i < 254; i++) {
      word p = oalloc(i, i);
      printf("%d: $%04x ", i, p);
    }
    odump();
  }
  ogc();
  odump();
}

void opanic(byte x) {
  fflush(stdout);
  fprintf(stderr, "\n*** PANIC %d\n", x);
  assert(0);
}

int main() {
  printf("hello %d %d\n", (int)sizeof(word), (int)sizeof(short));
  printf("oinit:\n");
  oinit(200, 50000, mark_no_roots);
  printf("did oinit\n");
  AllocAndPrintTest();
  printf("bye\n");
}
