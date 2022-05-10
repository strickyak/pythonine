#ifndef PYTH09__RUNTIME_H_
#define PYTH09__RUNTIME_H_

#include "_generated_prim.h"
#include "octet.h"
#include "osetjmp.h"
#include "pb2.h"
#include "readbuf.h"

// For olongjmp() to run_loop_jmp_buf:
enum { FINISH = 1, CONTINUE = 2 };

extern word StrFromC(const char* s);
extern byte InternString(word str);

extern void EvalCodes(word code);
#if !unix
extern asm void Intercept();
#endif
extern void SetIntercept();
extern void RunLoop();
extern bool StrEqual(word a, word b);
bool Truth(word a);

void RuntimeInit();
void MarkRoots();
void SlurpModule(struct ReadBuf*, word* bc_out);
void Break(const char* why);
void SayPyStack();

// GC Roots:
extern word ForeverRoot;
extern word RootForMain;  // For main() to use.
extern word LoopAllocRoot;  // For DictItems, etc.

extern word Builtins;
extern word GlobalDict;  // todo: Modules.
extern word InternList;
extern word ClassList;
extern word MessageList;
extern word DunderInitStr;
extern word DunderInitIsn;
extern word DunderIterIsn;
extern word NextIsn;
extern word StopIterationStr;

extern word function;
extern word ip;  // Stores in Frame as ip - function.
extern word fp;
extern word sp;  // Stores in Frame as sp - fp.
extern bool signalled;
extern ojmp_buf run_loop_jmp_buf;
extern ojmp_buf user_jmp_buf;

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

#define FRAME_SIZE 48  /* was 32 */
#define OBJECT_SIZE 32 /* TODO, use correct size */

#define Q(N) FROM_INT(N)
#define N(Q) TO_INT(Q)

// We really want all zero bits to mean None.
#define None ((word)0)

void SayStr(word p);
void SayObj(word p, byte level);

word MemberGet(word obj, byte isn);
void MemberPut(word obj, byte isn, word value);
word ArgGet(byte i);
void ArgPut(byte i, word a);
word FindMethForObjOrNull(word obj, byte meth_isn);
word SingletonStr(byte ch);
void RunBuiltin(byte builtin_num);
void Directory();

void Construct(byte cls_num, byte nargs);
void Call(byte nargs, word fn);
void CallMeth(byte meth_isn, byte nargs);
void Return(word retval);
void DoFor();
void DoNext();
void DoHandleStopIteration(byte end_while);
void DoTry(byte catch_loc);
void DoEndTry(byte end_catch_loc);
void ShedTryBlocks(byte n);
void SetJmp(word a);
void Implode(byte len, word chain);
int Len(word o);
void Explode(byte len);
word GetItem(word coll, word key);
word PutItem(word coll, word key, word value);
void Raise(word ex);
void RaiseC(const char* err);
word PopSp();
void PushSp(word a);
void SimplePrint(word p);
void DumpStats();
bool Equal(word a, word b);
void PleaseCallMeth0(byte meth_isn, word self);

#define FOR_EACH(I, ITEM, X)                           \
  {                                                    \
    struct TrainIterator iter;                         \
    TrainIterStart((X), &iter);                        \
    for (int I = 0; TrainIterMore(&iter); I++) { \
      word ITEM = TrainIterNext(&iter);
#define DO {
#define DONE \
  }          \
  }          \
  }

#define CHECK(PRED, ERR) \
  if (!(PRED)) {         \
    RaiseC(ERR);         \
  }

#endif  // PYTH09__RUNTIME_H_
