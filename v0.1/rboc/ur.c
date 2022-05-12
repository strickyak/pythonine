#include "ur.h"
#include "mmap.h"
#include "readbuf.h"

byte CurrentRegion;
byte RegionBlocks[16];

struct ReadBuf rb;
char read_buffer[READ_BUF_SIZE];
byte map_buffer[256];

void Diagnostics() {
  asm {
    SWI
      FCB 102
  }
  /*
  printf(" HW(");
  byte* p = (byte*)0xFFA0;
  for (byte i = 0; i < 16; i++) {
    printf("%02x ", p[i]);
    if ((i&3) == 3) printf(" ");
  }
  printf(")\n");
  */
}

#if 0
static void ShowBlockMap(const char* why) {
  // int GetBlockMap(byte* buf, word* bpb_out, word* num_blks_out)

  Diagnostics();
  word bpb, num_blks;
  GetBlockMap(map_buffer, &bpb, &num_blks);
  printf(" GetBlockMap->%x,%x (%s)\n", bpb, num_blks, why);
  for (byte i = 0; i < 8; i++) {
    byte something = 0;
    for (byte j=0; j<32; j++) {
      if (map_buffer[j + (word)i * 32]) something = 1;
    }
    if (something) {
      printf("%x: ", (word)i * 32);
      for (byte j=0; j<32; j++) {
        printf("%02x ", map_buffer[j + (word)i * 32]);
        if ((j&3) == 3) printf(" ");
      }
      printf("\n");
    }
  }
}
#endif

static void Map(byte b) {
  //ShowBlockMap("before Map");
  word addr;
  checkerr(MapBlock(RegionBlocks[b], 1, &addr));
  asserteq(addr, MAPPED_BLOCK_ADDR);
  //ShowBlockMap("after Map");
}
static void UnMap() {
  //ShowBlockMap("before UnMap");
  checkerr(UnMapBlock(MAPPED_BLOCK_ADDR, 1));
  //ShowBlockMap("after UnMap");
}

static byte Remap(byte new) {  // return previous region.
  byte old = CurrentRegion;
  if (old != new) {
    if (old) UnMap();
    if (new) Map(new);
    CurrentRegion = new;
  }
  return old;
}

void LoadRegion(byte region, const char* filename) {
  //ShowBlockMap("LR: first");
  word block = 0;
  checkerr(AllocateBlock(1, &block));
  assert(block < 255);
  RegionBlocks[region] = (byte)block;
  //ShowBlockMap("LR: before remap");
  Remap(region);
  //ShowBlockMap("LR: after remap");

  byte* p = (byte*)(MAPPED_BLOCK_ADDR); 
  // read file
  ReadBufOpen(&rb, filename, read_buffer);
  while (rb.end) { // end becomes 0 at EOF.
  // for (int i = 0; i < 256; i++) {
    byte b = ReadBufCurrent(&rb);
    // printf(" <%x,%x> ", p, b);
    *p++ = b;
    ReadBufNext(&rb);
  }
  ReadBufClose(&rb);
}

word CALLER(byte region, word address, byte num_args, ...) {
  word retval = 0;
  printf("TODO: call %x %x %x\n", region, address, num_args);

  byte old = Remap(region);
  asm {
    pshs y       ; dont change frame pointer u
    leax address
    leax -2,x
    ldb num_args ; loop variable
    abx
    abx          ; advance X to last arg.
    
    tstb
Caller0
    beq Caller1 ; num_args DO
    ldy 0,x
    pshs y     ; push last args first.
    leax -2,x
    decb
    bra Caller0 ; LOOP

Caller1
    puls y
    ldx address
    jsr 0,x
    std retval
  }

  Remap(old);  // Restore previous map.
  printf("TODO: return %x %x %x => %x\n", region, address, num_args, retval);
  return retval;
}

void Stop(const char* why) {
  printf(" *** STOPPING(%s)\n", why);
  asm {
    SWI
    FCB 100
  }
}
