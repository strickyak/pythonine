// runpy: Pythonine main bytecode interpreter.

#include "data.h"
#include "octet.h"
#include "readbuf.h"
#include "runtime.h"

#if !unix
#include "os9errno.h"
#include "os9.h"
#endif

// Temporary hack for loading one bytecode file at startup.
char bytecode_filename[32];

void main2() {
  defs_init(MarkRoots);
  InitData();
  RuntimeInit();

  struct ReadBuf read_buf;
  ReadBufOpen(&read_buf, bytecode_filename);

  word main_bytecodes;
  SlurpModule(&read_buf, &main_bytecodes);
  Directory();

  EvalCodes(main_bytecodes);
  ogc();
  odumpsummary();
  exit(0);
}

int main(int argc, char* argv[]) {
  assert(argc>1);
  assert(argv);
  assert(argv[1]);
  assert(strlen(argv[1]) < sizeof(bytecode_filename));
  strcpy(bytecode_filename, argv[1]);
#if !unix
  word old_size = 0;
  word wanted_size = 0x4000;
  word got_size = 0x4000;
  word new_size = 0;
  byte err = 0;
  word stack_pointer = 0;

  for (int i =0; i<5; i++) {

  asm {
    sts stack_pointer

    leay 0,y  ; sets Z flag.  Zero means Level 2 OS-9.
    beq IsLevel2

    ldb #E_BMODE   ; Cannot work on Level 1 OS-9.
    bra ReSizeError

IsLevel2
    clra  ; non means query
    clrb
    pshs y,u
    swi2
    fcb   F_MEM
    puls y,u
    bcs ReSizeError
    std old_size

    ldd wanted_size  ; nonzero means desire.
    pshs y,u
    swi2
    fcb   F_MEM
    puls y,u
    bcs ReSizeError
    std new_size
    bra ReSizeOk

ReSizeError
    stb err
ReSizeOk
  }
  // printf("err %d. old %x new %x\n", (int)err, old_size, new_size);

  if (err==0) got_size = new_size;

  wanted_size += 0x2000;
  }
  // printf("(data $%x) ", got_size);

  asm {
    tfr y,d     ; zero in D, X, and Y
    lds got_size
    pshs d,y
    pshs d,y
    tfr s,u     ; Start new high frame pointer
    pshs d,y
    pshs d,y
  }
#endif
  main2();
}
