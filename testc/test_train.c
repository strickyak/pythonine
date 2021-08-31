#include "standard.h"
#include "octet.h"
#include "train.h"
#include "defs.h"
#include "_generated_prim.h"

word r1;
word r2;
word r3;
void Mark() {
  omark(r1);
  omark(r2);
  omark(r3);
}

void One() {
  defs_init(Mark);
  r1 = NewTrain(32, C_List);

  for (int i = 0; i < 1000; i++) {
    assert(TrainLen2(r1) == i);
    TrainAppend(r1, FROM_INT(i+1000));
  }
  for (int i = 0; i < 1000; i++) {
    word x = TrainGetNth(r1, i);
    int y = TO_INT(x);
    assert(y == i+1000);
  }
  for (int i = 0; i < 10000; i++) {
    r2 = NewTrain(64, C_List);
    if ((i&63)==0) fflush(stdout);
  }
  for (int i = 0; i < 1000; i++) {
    word x = TrainGetNth(r1, i);
    int y = TO_INT(x);
    assert(y == i+1000);
  }
}

int main() {
    One();
    printf("OKAY.\n");
}
