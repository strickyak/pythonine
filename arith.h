#ifndef PYTHONINE_ARITH_H_
#define PYTHONINE_ARITH_H_

#include "octet.h"
#include "runtime.h"

typedef int (*BinaryIntOp)(int a, int b);
typedef word (*BinaryPyOp)(word a, word b);

extern word bad_pyfn(word a, word b);

extern int int_plus(int a, int b);
extern int int_minus(int a, int b);
extern int int_times(int a, int b);
extern int int_shiftl(int a, int b);
extern int int_shiftru(int a, int b);
extern int int_shiftrs(int a, int b);
extern int int_eq(int a, int b);
extern int int_ne(int a, int b);
extern int int_lt(int a, int b);
extern int int_gt(int a, int b);
extern int int_le(int a, int b);
extern int int_ge(int a, int b);

#endif
