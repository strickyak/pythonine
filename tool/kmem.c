#include <cmoc.h>
#include <assert.h>

#include "mmap.h"

#define NUM_BLOCKS 40

byte BlockNum[NUM_BLOCKS];
byte Buffer[1024];

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
        POKE(0xFFA0 + 8 + 6, b);
        assert( *(byte*)0xC000 == 0xDE ^ b);
        Enable();
  }
  return 0;
}
