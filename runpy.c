// runpy: Pythonine main bytecode interpreter.

#include "data.h"
#include "octet.h"
#include "readbuf.h"
#include "runtime.h"

int main(int argc, char* argv[]) {
  defs_init(MarkRoots);
  InitData();
  RuntimeInit();

  struct ReadBuf read_buf;
  ReadBufOpen(&read_buf, "bc");

  word main_bytecodes;
  SlurpModule(&read_buf, &main_bytecodes);
  Directory();

  EvalCodes(main_bytecodes);
  printf("\n[[[ FINISHED ]]]\n");
  DumpStats();
  ogc();
  DumpStats();
  return 0;
}
