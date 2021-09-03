#ifndef PYTHONINE_DEFS_H_
#define PYTHONINE_DEFS_H_

#include "octet.h"

#ifndef STACK_GAP
#define STACK_GAP 300
#endif

extern void defs_init(void (*marker_fn)());

extern byte ToByte(int x);
extern bool IS_INT(word x);
extern bool IS_INT2(word x, word y);
extern int TO_INT(word x);
extern word FROM_INT(int x);
void opanic(byte x);

#endif  // PYTHONINE_DEFS_H_
