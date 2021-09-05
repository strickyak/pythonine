#include <cmoc.h>

#define F_GBlkMp 0x19

typedef unsigned char byte;
typedef unsigned int word;

byte Buffer[1024];

void Disable() {
  asm {
    orcc    #$50
  }
}

void Enable() {
  asm {
    orcc    #$AF
  }
}

byte AllocateRam(byte num_blks, word* start_blk_out) {
  byte err = 0;
  asm {
    PSHS Y,U
    LDB num_blks
    os9 $39
    PULS Y,U
    bcc AR__OKAY

AR__BAD
    stb err
    bra AR__RET

AR__OKAY
    std [start_blk_out]

AR__RET
    nop
  }
  return err;
}

byte MapBlock(int start_blk, byte num_blks, word* addr_out) {
  byte err = 0;
  asm {
    PSHS Y,U
    LDB num_blks
    LDX start_blk
    os9 $4F
    TFR U,X
    PULS Y,U
    bcc MB__OKAY

MB__BAD
    stb err
    bra MB__RET

MB__OKAY
    stx [addr_out]

MB__RET
    nop
  }
  return err;
}

int GetBlockMap(byte* buf, word* bpb_out, word* num_blks_out) {
  int err = 0;
  asm {
    PSHS Y
    LDX buf
    PSHS U
    os9 F_GBlkMp
    PULS U
    bcc GVM__OKAY

GVM__BAD
    PULS Y
    clra
    std err
    bra GVM__RET

GVM__OKAY
    TFR Y,X
    PULS Y
    stD [bpb_out]
    stX [num_blks_out]

GVM__RET
    nop
  }
  return err;
}

void PrintBlockMap() {
  word bpb = 0;
  word num_blks = 0;
  int err = GetBlockMap(Buffer, &bpb, &num_blks);
  if (err) { printf("ERROR %d\n", err); return; }

  printf("bpb: %u\n", bpb);
  printf("num_blks: %u\n", num_blks);
  for (int i=0; i < 256; i+=16) {
    for (int j=0; j<16; j++) {
      printf(" %02x", Buffer[i+j]);
    }
    printf("\n");
  }
}

int main() {
  PrintBlockMap();

  for (int i=0; i < 16; i++) {
    word start_blk, addr;
    printf("allocate ram[%d] ... ", i);
    byte err = AllocateRam(1, &start_blk);
    if (err) { printf("ERROR %d\n", err); return err; }
    printf("=> blk %d; ", start_blk);
    err = MapBlock(start_blk, 1, &addr);
    if (err) { printf("ERROR %d\n", err); }
    printf(" MAP addr => %x\n", addr);
    PrintBlockMap();
  }

  return 0;
}
