#include "defs.h"

#include "arith.h"
#include "octet.h"

#define PRIM_PART 2
#include "_generated_prim.h"
#undef PRIM_PART

bool IS_INT(word x) { return (byte)1 & (byte)x; }

bool IS_INT2(word x, word y) { return (byte)1 & (byte)x & (byte)y; }

int TO_INT(word x) {
  assert1(x & 1u, "TO_INT %04x", x);
  if (x & 0x8000) {
    // neg
    return 0x4000 | 0x3FFF & -(int)(x >> 1);
  } else {
    // pos
    return 0x3FFF & (x >> 1);
  }
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

#define MEMSIZE 32000u
// #define MEMSIZE 25000u

#if unix
#define data 10u
#define data_end (data + MEMSIZE)
#else
char data_array[MEMSIZE];
#define data ((unsigned)data_array)
#define data_end (data + MEMSIZE)
#endif

void defs_init(void (*marker_fn)()) {
#if unix
  printf("oinit: start=%04x end=%04x markerfn=%x\n", data, data_end,
         (unsigned)(unsigned long)marker_fn);
#else
  printf("oinit: start=%04x end=%04x markerfn=%x\n", data, data_end,
         (unsigned)marker_fn);
#endif
  oinit(data, data_end, marker_fn);
  odump(0, 0, 0, 0);
}

#endif  // ========================================

#ifdef TEST___JUST_FOR_EXAMINING_THE_ASSEMBLY_OUTPUT

int xxx;

struct Pair {
    word up;
    word down;
};

struct HiLo {
    byte hi;
    byte lo;
};

word TestWord(word a, word b) {
    word c;
    c = 1;
    {
        word d;
        d = 4 + a;
        xxx += d;
    }
    {
        word e;
        e = 6 + b;
        xxx += e;
    }
    return a+b+c;
}

byte TestByte(byte a, byte b) {
    byte c;
    c = 1;
    {
        byte d;
        d = 4 + a;
        xxx += d;
        printf("%d", d);
        printf("%d", &d);
    }
    {
        byte e;
        e = 6 + b;
        xxx += e;
        printf("%d", e);
        printf("%d", &e);
    }
    return a+b+c;
}

struct Pair TestPair(struct Pair a, struct Pair b) {
    struct Pair foo = {a.up+a.up+3, b.down+b.down+5};
    return foo;
}
#endif
