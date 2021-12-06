#include "standard.h"
#include "octet.h"
#include "train.h"
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

void TestTrain1() {
  defs_init(Mark);
  r1 = NewTrain(32, C_List);

  // Add 1000 elements.
  for (int i = 0; i < 1000; i++) {
    assert(TrainLen2(r1) == i);
    TrainAppend(r1, FROM_INT(i+1000));
  }
  // Check with Get Nth
  for (int i = 0; i < 1000; i++) {
    word x = TrainGetNth(r1, i);
    int y = TO_INT(x);
    assert(y == i+1000);
  }
  // Create garbage.
  for (int i = 0; i < 2000; i++) {
    // This will require some garbage collections.
    r2 = NewTrain(64, C_List);
    if ((i&63)==0) fflush(stdout);
  }
  for (int i = 0; i < 1000; i++) {
    word x = TrainGetNth(r1, i);
    int y = TO_INT(x);
    assert(y == i+1000);
  }
  // FOR EACH
  {
      FOR_EACH(i, item, r1) DO
        assert(TO_INT(item) == i+1000);
      DONE
  }
  // Iterator
  {
    struct TrainIterator it;
    int i = 0;
    TrainIterStart(r1, &it);
    while (TrainIterMore(&it)) {
      word e = TrainIterNext(&it);
      assert(TO_INT(e) == i+1000);
      i++;
    }
  }
  // Put Nth
  for (int i = 0; i < 111; i++) {
    TrainPutNth(r1, i, FROM_INT(i+i));
  }
  // GetNth
  for (int i = 0; i < 111; i++) {
    word x = TrainGetNth(r1, i);
    int y = TO_INT(x);
    assert(y == i+i);
  }

  r2 = NewTrainIter(r1, C_ListIter);
}

int main() {
    TestTrain1();
    odumpsummary();
    ogc();
    odumpsummary();
    printf("\nOKAY.\n");
}
