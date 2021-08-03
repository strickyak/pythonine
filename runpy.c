// runpy: Pythonine main bytecode interpreter.

#include "octet.h"
#include "readbuf.h"
#include "runtime.h"

// extern const byte const BuiltinClassMessageMeths[];

int main(int argc, char* argv[]) {
  {
    const byte* q = BuiltinClassMessageMeths;
    printf("\nINIT(a): q=%x : %x %x %x %x %x %x\n", q, q[0], q[1], q[2], q[3],
           q[4], q[5]);
    assert(q[0] == 10);
    assert(q[2] == 10);
    assert(q[4] == 0);
  }

  defs_init(MarkRoots);
  RuntimeInit();

  struct ReadBuf read_buf;
  ReadBufOpen(&read_buf, "bc");

  word main_bytecodes;
  SlurpModule(&read_buf, &main_bytecodes);
  Directory();

  EvalCodes(main_bytecodes);
  printf("\n[[[ FINISHED ]]]\n");
  return 0;
}
