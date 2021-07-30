#ifndef PYTH09__RUNTIME_H_
#define PYTH09__RUNTIME_H_

#include "_generated_core.h"
#include "octet.h"
#include "pb2.h"
#include "readbuf.h"

#define GetB(addr) ogetb(addr)
#define GetW(addr) ogetw(addr)
#define PutB(addr, x) oputb(addr, x)
#define PutW(addr, x) oputw(addr, x)

#if 0
extern byte GetB(word a);
extern void PutB(word a, byte x);
extern word GetW(word a);
extern void PutW(word a, word x);
#endif

#if 0
extern word NewBuf(); // unused for now
#endif
extern word NewStr(word obj, byte off, byte len);
extern word NewStrCopyFrom(word s, byte len);
extern word NewStrCopyFromC(const char* s);
extern void EvalCodes(word code);
extern void RunLoop();
extern bool StrEq(word a, word b);
bool Truth(word a);

void RuntimeInit();
void MarkRoots();
void SlurpModule(struct ReadBuf*, word* bc_out);
void Break();

// GC Roots:
extern word ForeverRoot;
extern word RootForMain;  // For main() to use.

#define STACK_SIZE 200
extern word Builtins;
extern word GlobalDict;  // todo: Modules.
extern word InternList;
extern word ClassList;

extern word function;
extern word ip;  // Stores in Frame as ip - function.
extern word fp;
extern word sp;  // Stores in Frame as sp - fp.

// Header on bytecodes object:
#define BC_NUM_ARGS 0
#define BC_NUM_LOCALS 1
#define BC_NUM_TEMPS 2
#define BC_MODULE 3
#define BC_CLASS 4
#define BC_NAME 5
#define BC_HEADER_SIZE 6

// Frame Format
#define FR_START_ARGS -2
#define FR_CODES_OBJ -1
#define FR_OLD_FP 0
#define FR_MAGIC 1
#define FR_NARGS 2
#define FR_OLD_IP 3
#define FR_START_LOCALS 4

#define MAGIC_FRAME_VALUE 0x1555
#define MAGIC_FRAME_OOP FROM_INT(MAGIC_FRAME_VALUE)

#define Q(N) FROM_INT(N)
#define N(Q) TO_INT(Q)

// We really want all zero bits to mean None.
#define None ((word)0)

void SayStr(word p);
void SayObj(word p, byte level);

word MemberGet(word obj, byte isn);
void MemberPut(word obj, byte isn, word value);
word ArgGet(byte i);
word FindMethForObjOrNull(word obj, byte meth_isn);
word SingletonStr(byte ch);
void RunBuiltinMethod(byte meth_num);

word Construct(byte cls_num, byte nargs);
void Call(byte nargs);
void CallMeth(byte meth_isn, byte nargs);

#endif  // PYTH09__RUNTIME_H_
