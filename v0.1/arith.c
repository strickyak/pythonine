#include "arith.h"

word CallIntOrPy(word a, word b, BinaryIntOp intfn, BinaryPyOp pyfn) {
  if (IS_INT2(a, b)) {
    return FROM_INT(intfn(TO_INT(a), TO_INT(b)));
  }
  return pyfn(a, b);
}

word bad_pyfn(word a, word b) {
  assert0(0, "bad_pyfn");
  return 0;
}

int int_plus(int a, int b) { return a + b; }
int int_minus(int a, int b) { return a - b; }
int int_times(int a, int b) { return a * b; }
int int_shiftl(int a, int b) { return a << b; }
int int_shiftru(int a, int b) { return (unsigned)a >> (unsigned)b; }
int int_shiftrs(int a, int b) { return a >> b; }
int int_eq(int a, int b) { return a == b; }
int int_ne(int a, int b) { return a != b; }
int int_lt(int a, int b) { return a < b; }
int int_gt(int a, int b) { return a > b; }
int int_le(int a, int b) { return a <= b; }
int int_ge(int a, int b) { return a >= b; }
