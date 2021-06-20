#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chain.h"
#include "runtime.h"

word chain1;

void mark_roots() {
  if (chain1) omark(chain1);
}

void Test1() {
  oinit(500, 50000, mark_roots);
  chain1 = NewList();
  for (byte i = 0; i < 254; i++) {
    word a = ChainAddrOfAppend(chain1);
    assert(a);
    StartQ(a, i);
  }
  for (byte i = 0; i < 254; i++) {
    word a = ChainAddrOfNth(chain1, i);
    byte q = GetQ(a);
    assert(q == i);
    word b = ChainGetNth(chain1, i);
    assert(N(b) == q);
  }
}

int main(int argc, char* argv[]) { Test1(); }
