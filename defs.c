#include "defs.h"
#include "arith.h"
#include "octet.h"

#define _CORE_PART_ 2
#include "_generated_core.h"
#undef _CORE_PART_

bool IS_INT(word x) { return (byte)1 & (byte)x; }

bool IS_INT2(word x, word y) { return (byte)1 & (byte)x & (byte)y; }

int TO_INT(word x) {
  assert1(x & 1u, "TO_INT %04x", x);
  return (int)(x >> 1);
}

word FROM_INT(int x) {
  word y = (word)x;
  assert1(!(y & 0x8000), "FROM_INT %04x", y);
  return 1u | (y << 1);
}

void opanic(byte x) {
#if unix
  fflush(stdout);
  fprintf(stderr, "\n*** PANIC %d\n", x);
#else
  printf("\n*** PANIC %d\n", x);
#endif
  assert(0);
}

#if 1  // ========================================

#define MEMSIZE 20000u

#if unix
#define data 10u
#define data_end (data + MEMSIZE)
#else
char data_array[MEMSIZE];
#define data ((unsigned)data_array)
#define data_end (data + MEMSIZE)
#endif

void defs_init(void (*marker_fn)()) {
  printf("oinit: start=%04x end=%04x markerfn=%x\n", data, data_end,
         (unsigned)(unsigned long)marker_fn);
  oinit(data, data_end, marker_fn);
  odump(0, 0, 0, 0);
}
#endif  // ========================================
