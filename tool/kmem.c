#include <cmoc.h>
#include <assert.h>

#define NUM_BLOCKS 32

#define F_AllRAM 0x39
#define F_GBlkMp 0x19
#define F_ClrBlk 0x50
#define F_MapBlk 0x4F

typedef unsigned char byte;
typedef unsigned int word;

byte BlockNum[NUM_BLOCKS];
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

void Poke(word addr, byte val) {
    *(byte*)addr = val;
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

byte MapBlock(int start_blk, byte num_blks, word* addr_out) {
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

void PrintBlockMap() {
  word bpb = 0;
  word num_blks = 0;
  int err = GetBlockMap(Buffer, &bpb, &num_blks);
  if (err) { printf("ERROR %d\n", err); return; }

  printf("bpb: %u num_blks: %u\n", bpb, num_blks);
  for (int i=0; i < (num_blks+15); i+=16) {
    printf("%04x:", i);
    for (int j=0; j<16; j++) {
      printf(" %02x", Buffer[i+j]);
    }
    printf("\n");
  }
}

int main() {
  printf("main=$%04x\n", (unsigned)(void*)main);
  PrintBlockMap();

  for (int i=0; i < NUM_BLOCKS; i++) {
    word start_blk, addr;
    printf("allocate ram[%d] ... ", i);
    byte err = AllocateBlock(1, &start_blk);
    if (err) { printf("ERROR %d\n", err); return err; }
    BlockNum[i] = start_blk;
    printf("=> blk %d; ", start_blk);
    err = MapBlock(start_blk, 1, &addr);
    printf(" MAP addr => %x; error %d\n", addr, err);
    PrintBlockMap();

    if (!err && (i%3)>0) {
        err = UnMapBlock(addr, 1);
        printf(" UNMAP addr: %x ; error %d\n", addr, err);
        PrintBlockMap();
    }
  }

  printf("Part Two\n");
  for (word addr = 0x2000; addr < 0xE000; addr += 0x2000) {
        byte err = UnMapBlock(addr, 1);
        printf(" UNMAP addr: %x ; error %d\n", addr, err);
  }

  printf("Part Three\n");
  for (word k = 0; k < NUM_BLOCKS; k++) {
        byte b = BlockNum[k];
        word addr;
        byte err = MapBlock(b, 1, &addr);
        printf("k=%d MAP start: %d addr: %x ; error %d\n", k, b, addr, err);
        *(byte*)addr = 0xDE ^ b;
        err = UnMapBlock(addr, 1);
        printf("...  UNMAP addr: %x ; error %d\n", addr, err);
  }

  printf("Part Four\n");
  for (word k = 0; k < NUM_BLOCKS; k++) {
        byte b = BlockNum[k];
        word addr;
        byte err = MapBlock(b, 1, &addr);
        printf("k=%d MAP start: %d addr: %x ; error %d\n", k, b, addr, err);
        assert( *(byte*)addr == 0xDE ^ b);
        err = UnMapBlock(addr, 1);
        printf("...  UNMAP addr: %x ; error %d\n", addr, err);
  }

  printf("Part Five\n");
  for (word k = 0; k < NUM_BLOCKS; k++) {
        byte b = BlockNum[k];
        printf("POKE: k=%d MAP start: %d \n", k, b );
        Disable();
        Poke(0xFFA0 + 8 + 6, b);
        assert( *(byte*)0xC000 == 0xDE ^ b);
        Enable();
  }
  return 0;
}
