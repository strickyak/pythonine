#include "chain.h"
#include "arith.h"
// #include "runtime.h"
#include "defs.h"

word chain1;

void mark_roots() {
  if (chain1) omark(chain1);
}

#define NUM 10  // 254

void Test1() {
  defs_init(mark_roots);

  chain1 = NewList(false);
  for (byte i = 0; i < NUM; i++) {
    ChainAppend(chain1, FROM_INT((int)i));
    odump(0, 0, 0, 0);
  }
  odump(0, 0, 0, 0);
  for (byte i = 0; i < NUM; i++) {
    word a = ChainAddrOfNth(chain1, i);
    int q = TO_INT(GetW(a));
    assert(q == (int)i);
    word b = ChainGetNth(chain1, i);
    assert(TO_INT(b) == q);
  }
}

int main(int argc, char* argv[]) {
  Test1();
  return 0;
}
