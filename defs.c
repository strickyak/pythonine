#include "defs.h"

#include "arith.h"
#include "octet.h"

#define PRIM_PART 2
#include "_generated_prim.h"
#undef PRIM_PART

byte ToByte(int x) {
  byte b = (byte)x;
  assert((int)b == x);
  return b;
}
bool IS_INT(word x) { return (byte)1 & (byte)x; }

bool IS_INT2(word x, word y) { return (byte)1 & (byte)x & (byte)y; }

int TO_INT(word oop) {
  assert1(oop & 1u, "TO_INT %04x", oop);

  // Shift encoded oop to left 1, to fix the top (sign) bit.
  // Then shift back to correct int value.
  return ((short)oop<<1)>>2;
}

word FROM_INT(int x) {
  return 1u | ((word)x << 1);
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

void defs_init(void (*marker_fn)()) {
#if unix
  #define MEMSIZE 32000u
  #define data 10u
  #define data_end (data + MEMSIZE)
  printf("oinit: start=$%04x end=$%04x\n", data, data_end);
#else
  word stack_ptr;
  asm {
    sts stack_ptr
  }
  word bss_max;
  asm {
    IMPORT l_bss
    ldd #l_bss
    std bss_max
  }

  word data = 0xFFFC & (bss_max + 8);
  word data_end = 0xFFFC & (stack_ptr - STACK_GAP);
  printf("oinit: size=%d. start=$%04x end=$%04x\n", (data_end - data), data, data_end);
#endif
  oinit(data, data_end, marker_fn);
  odump(0, 0, 0, 0);
}
