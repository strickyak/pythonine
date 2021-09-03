#include "data.h"
#include "defs.h"
#include "runtime.h"
#include "_generated_prim.h"

word r1;
word r2;
word r3;
void Mark() {
  omark(r1);
  omark(r2);
  omark(r3);
}

struct {
  int value;
  word encoded;
} const Cases[] = {
  { 0, 1 },
  { 1, 3 },
  { 2, 5 },
  { 100, 201 },
  { 101, 203 },
  { 0x1fff, 0x3fff },
  { 0x3fff, 0x7fff },
  { -1, 0xffff },
  { -2, 0xfffd },
};

void TestInt1() {
  defs_init(Mark);

  for (int i=0; i < (sizeof Cases)/(sizeof Cases[0]); i++) {
    printf("case %d, expecting %d ...\n", Cases[i].value, Cases[i].encoded);
    // frontwards, from int to encoded oop.
    asserteq(Cases[i].encoded, FROM_INT(Cases[i].value));

    // backwards, from encoded oop to int.
    asserteq(Cases[i].value, TO_INT(Cases[i].encoded));
  }
}

int main() {
    TestInt1();
    printf("\nOKAY.\n");
}
