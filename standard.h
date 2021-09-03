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

#define asserteq(A,B) do { word _a_ = (word)(A); word _b_ = (word)(B); \
                         if (_a_ != _b_) { \
                        fprintf(stderr, "@@@ ASSERT FAILED %s:%u: (%s) == (%s): but %d != %d\n", __FILE__, __LINE__, #A, #B, _a_, _b_); \
                        abort(); } } while (0)
#else

#define CAREFUL 0
#define GUARD 0

#define assert(cond) do { if (!(cond)) { \
                        printf("@@@ ASSERT FAILED %s:%u: %s\n", __FILE__, __LINE__, #cond); \
                        fatal_coredump(); \
                        for (;;); } } while (0)

#define asserteq(A,B) do { word _a_ = (word)(A); word _b_ = (word)(B); \
                         if (_a_ != _b_) { \
                        printf("@@@ ASSERT FAILED %s:%u: (%s) == (%s): but %d != %d\n", __FILE__, __LINE__, #A, #B, _a_, _b_); \
                        fatal_coredump(); \
                        for (;;); } } while (0)

#include <cmoc.h>

#define fflush(F) /*nothing*/

#endif

#endif
