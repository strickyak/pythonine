// runpy: Pythonine main bytecode interpreter.

#include "data.h"
#include "octet.h"
#include "readbuf.h"
#include "runtime.h"

#if !unix
#include "os9errno.h"
#include "os9.h"
#endif

void main2() {
#if !unix
  // printf("(main) $%x=%d. ", main, main);
#endif

  defs_init(MarkRoots);
  InitData();
  RuntimeInit();

  struct ReadBuf read_buf;
  ReadBufOpen(&read_buf, "bc");

  word main_bytecodes;
  SlurpModule(&read_buf, &main_bytecodes);
  Directory();

  EvalCodes(main_bytecodes);
  ogc();
  odumpsummary();
  exit(0);
}

int main(int argc, char* argv[]) {
#if !unix
  word old_size = 0;
  word wanted_size = 0x8000;
  word new_size = 0;
  byte err = 0;
  word stack_pointer = 0;

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
  printf("err %d. old %x new %x\n", (int)err, old_size, new_size);
  if (err) exit(err);

  asm {
    tfr y,d     ; zero in D, X, and Y
    tfr y,x
    lds #$8000  ; Start new high stack
    pshs d,x,y
    pshs d,x,y
    tfr s,u     ; Start new high frame pointer
    pshs d,x,y
    pshs d,x,y
  }
#endif
  main2();
}
