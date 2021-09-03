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

void TestDict1() {
  defs_init(Mark);
  r1 = NewDict();

  // Add 150 elements.
  for (int i = 0; i < 150; i++) {
    assert(DictLen(r1) == i);
    DictPut(r1, FROM_INT(i), FROM_INT(i+i+i));
  }
  // Check with DictGet
  for (int i = 0; i < 150; i++) {
    word x = DictGet(r1, FROM_INT(i));
    int y = TO_INT(x);
    assert(y == i+i+i);
  }
  // Create garbage.
  for (int i = 0; i < 2000; i++) {
    // This will require some garbage collections.
    r2 = NewTrain(64, C_List);
    if ((i&63)==0) fflush(stdout);
  }
  // Check with DictGet
  for (int i = 0; i < 150; i++) {
    word x = DictGet(r1, FROM_INT(i));
    int y = TO_INT(x);
    assert(y == i+i+i);
  }
}

int main() {
    TestDict1();
    odumpsummary();
    ogc();
    odumpsummary();
    printf("\nOKAY.\n");
}
