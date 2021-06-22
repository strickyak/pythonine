// runpy: Pythonine main bytecode interpreter.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "runtime.h"

word pb;
void mark_roots() {
  if (pb) omark(pb);
  if (GlobalDict) omark(GlobalDict);
}

word ByteArrayToBytesObj(byte* a, size_t sz) {
  word p = oalloc(sz, C_Bytes);
  for (word i = 0; i < sz; i++) {
    oputb(p + i, a[i]);
  }
  return p;
}

int main(int argc, char* argv[]) {
  oinit(500, 50000, mark_roots);
  RuntimeInit();

  FILE* input = stdin;
  if (argc > 1) {
    input = fopen(argv[1], "r");
    if (!input) {
      perror(argv[1]);
      exit(7);
    }
  }

  pb = oalloc(254, C_Bytes);
  ozero(pb, 254);  // should be redundant.
  size_t n = fread(olea(pb), 1, 254, input);
  if (n <= 0) {
    fprintf(stderr, "\n*Error* cannot read bytecodes\n");
    exit(2);
  }
  word main_bytecodes;
  GlobalDict = SlurpModule(pb, &main_bytecodes);
  EvalCodes(main_bytecodes);
  fprintf(stderr, "\n      [ [ [ FINISHED ] ] ]\n");
}
