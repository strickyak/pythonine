#ifndef _MMAP_H_
#define _MMAP_H_

#ifndef _TYPEDEF_BYTE_
#define _TYPEDEF_BYTE_
typedef unsigned char byte;
typedef unsigned int word;
#endif

void Disable();  // interrupts.
void Enable();  // interrupts.
byte Peek(word addr);
void Poke(word addr, byte val);
byte AllocateBlock(byte num_blks, word* start_blk_out);
byte MapBlock(word start_blk, byte num_blks, word* addr_out);
byte UnMapBlock(word addr, byte num_blks);
int GetBlockMap(byte* buf, word* bpb_out, word* num_blks_out);

#endif
