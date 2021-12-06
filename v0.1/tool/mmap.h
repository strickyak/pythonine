#ifndef _MMAP_H_
#define _MMAP_H_

#include "types.h"

asm void Disable();  // interrupts.
asm void Enable();  // interrupts.
byte AllocateBlock(byte num_blks, word* start_blk_out);
byte MapBlock(word start_blk, byte num_blks, word* addr_out);
byte UnMapBlock(word addr, byte num_blks);
int GetBlockMap(byte* buf, word* bpb_out, word* num_blks_out);

#define PEEK(ADDR) (*(byte*)(ADDR))
#define POKE(ADDR,VAL) (*(byte*)(ADDR) = (VAL))

#endif
