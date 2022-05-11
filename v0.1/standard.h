#ifndef PYTHONINE_STANDARD_H_
#define PYTHONINE_STANDARD_H_

#ifdef unix

#define CAREFUL 1
#define GUARD 1
#define DEBUG 1

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define checkerr(E) do { byte _e_ = (E); if (_e_) { \
                        fprintf(stderr, "@@@ checkerr %x=%d. FAILED %s:%u\n", _e_, _e_, __FILE__, __LINE__); \
                        abort(); } } while (0)

#define asserteq(A,B) do { word _a_ = (word)(A); word _b_ = (word)(B); \
                         if (_a_ != _b_) { \
                        fprintf(stderr, "@@@ ASSERT FAILED %s:%u: (%s) == (%s): but %d != %d\n", __FILE__, __LINE__, #A, #B, _a_, _b_); \
                        abort(); } } while (0)
#else

#define CAREFUL 1
#define GUARD 1

#define checkerr(E) do { byte _e_ = (E); if (_e_) { \
                        printf("@@@ checkerr %x=%d. FAILED %s:%u\n", _e_, _e_, __FILE__, __LINE__); \
                        fatal_coredump(); \
                        exit(13); } } while (0)

#define assert(cond) do { if (!(cond)) { \
                        printf("@@@ ASSERT FAILED %s:%u\n", __FILE__, __LINE__); \
                        fatal_coredump(); \
                        exit(13); } } while (0)

#define asserteq(A,B) do { word _a_ = (word)(A); word _b_ = (word)(B); \
                         if (_a_ != _b_) { \
                        printf("@@@ ASSERT FAILED %s:%u: (%s) == (%s): but %d != %d\n", __FILE__, __LINE__, #A, #B, _a_, _b_); \
                        fatal_coredump(); \
                        exit(13); } } while (0)

#include <cmoc.h>

#define fflush(F) /*nothing*/

#endif

#define assert0(C, F) \
  if (!(C)) {printf(F); fflush(stdout); assert(0);}
#define assert1(C, F, X) \
  if (!(C)) {printf((F), (X)); fflush(stdout); assert(0);}
#define assert2(C, F, X, Y) \
  if (!(C)) {printf((F), (X), (Y)); fflush(stdout); assert(0);}
#define opanic(X) assert1(0, "\n*** opanic(%d)\n", (X))

#endif
