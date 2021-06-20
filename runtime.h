#ifndef PYTH09__RUNTIME_H_
#define PYTH09__RUNTIME_H_

#include "octet.h"

#include "_generated_core.h"

#if 0

#define GetB(addr) ogetb(addr)
#define GetW(addr) ogetw(addr)
#define PutB(addr, x) oputb(addr, x)
#define PutW(addr, x) oputw(addr, x)
#define StartQ(addr, x) oputw(0xFF | (byte)(x))
#define GetQ(addr) ogutb((addr) + 1)
#define PutQ(addr, x) oputb((addr) + 1, (byte)(x))

#else

extern byte GetB(word a);
extern void PutB(word a, byte x);
extern word GetW(word a);
extern void PutW(word a, word x);
extern void StartQ(word a, byte x);
extern byte GetQ(word a);
extern void PutQ(word a, byte x);

#endif

extern word new_flex(byte main_cls, byte guts_cls);
extern bool IsQ(word addr);
extern byte VecSize2(word coll);
extern void VecResize2(word coll, word sz2);
extern word NewBuf();
extern word NewStr(word obj, byte off, byte len);
extern word ListGetAt(word obj, byte at);
extern void ListPutAt(word obj, byte at, word value);
extern void EvalCodes(word code);
extern bool StrEq(word a, word b);
bool Truth(word a);

void RuntimeInit();
word SlurpModule(word p, word* bc_out);

#define STACK_SIZE 100
extern word Builtins;
extern word GlobalDict;  // todo: Modules.
extern word InternList;
extern word ClassList;
word Stack[STACK_SIZE];
extern int sp;

// Wrap N as a pseudo-pointer.
#define Q(N) ((word)0xFF00 | (word)(N))
// Decode a pseudo-pointer to the byte N.
#define N(Q) ((byte)((Q)&0xFF))

// We really want all zero bits to mean None.
#define None ((word)0)

#endif  // PYTH09__RUNTIME_H_
