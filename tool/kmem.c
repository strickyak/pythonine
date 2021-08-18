#include <cmoc.h>

#define F_GBlkMp 0x19

typedef unsigned char byte;
typedef unsigned int word;

byte Buffer[1024];

int GetBlockMap(byte* buf, word* bpb_out, word* num_blks_out) {
  int rv = 0;
  asm {
    PSHS Y
    LDX buf
    PSHS U
    os9 F_GBlkMp
    PULS U
    bcc OKAY

BAD
    clra
    std rv
    bra RET

OKAY
    stD bpb_out
    stY num_blks_out
    clra
    clrb
    stD rv

RET
    PULS Y
  }
  return rv;
}

int main() {
  word bpb = 0;
  word num_blks = 0;
  int err = GetBlockMap(Buffer, &bpb, &num_blks);
  if (err) return err;

  printf("bpb: %u\n", bpb);
  printf("num_blks: %u\n", num_blks);
  for (int i=0; i < 256; i+=16) {
    for (int j=0; j<16; j++) {
      printf(" %02x", Buffer[i+j]);
    }
    printf("\n");
  }

  return 0;
}
