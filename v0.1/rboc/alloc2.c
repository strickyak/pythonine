#include "prog0.h"
#include "stringy1.h"
#include "alloc2.h"

#include "region.2.h"

void MemInit() {
  printf("MemInit.\n");
  Next = 0;
}

char* Malloc(int n) {
  char* z = MemoryPool + Next;
  Next += n;
  return z;
}
