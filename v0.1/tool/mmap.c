#include "mmap.h"

#define F_AllRAM 0x39
#define F_GBlkMp 0x19
#define F_ClrBlk 0x50
#define F_MapBlk 0x4F

asm void Disable() {
  asm {
    orcc    #$50
  }
}

asm void Enable() {
  asm {
    andcc    #$AF
  }
}

byte AllocateBlock(byte num_blks, word* start_blk_out) {
  byte err = 0;
  asm {
    PSHS Y,U
    LDB num_blks
    os9 F_AllRAM
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

byte MapBlock(word start_blk, byte num_blks, word* addr_out) {
  byte err = 0;
  asm {
    PSHS Y,U
    LDB num_blks
    LDX start_blk
    os9 F_MapBlk
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
byte UnMapBlock(word addr, byte num_blks) {
  byte err = 0;
  asm {
    PSHS Y,U
    LDB num_blks
    LDU addr
    os9 F_ClrBlk
    PULS Y,U
    bcc UMB__OKAY

UMB__BAD
    stb err
    bra UMB__RET

UMB__OKAY
UMB__RET
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
