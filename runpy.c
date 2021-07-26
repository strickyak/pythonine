// runpy: Pythonine main bytecode interpreter.

#include "octet.h"
#include "readbuf.h"
#include "runtime.h"

int main(int argc, char* argv[]) {
  defs_init(MarkRoots);
  RuntimeInit();

  struct ReadBuf read_buf;
  ReadBufOpen(&read_buf, "bc");

  word main_bytecodes;
  SlurpModule(&read_buf, &main_bytecodes);

  EvalCodes(main_bytecodes);
  printf("\n\n   [ [ [ FINISHED ] ] ] \n\n");
  return 0;
}
