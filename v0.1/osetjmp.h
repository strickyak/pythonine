#ifndef PYTHONINE_OSETJMP_H_
#define PYTHONINE_OSETJMP_H_

#include "octet.h"
#include "standard.h"

#if unix

#include "setjmp.h"

#define ojmp_buf jmp_buf
#define osetjmp setjmp
#define olongjmp longjmp

#else

typedef word ojmp_buf[4];

asm word osetjmp(ojmp_buf buf);
asm word olongjmp(ojmp_buf buf, word retval);

#endif  // unix

#endif
