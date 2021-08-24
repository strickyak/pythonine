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

#else

#define CAREFUL 0
#define GUARD 0


#define assert(cond) do { if (!(cond)) { \
                        printf("@@@ ASSERT FAILED (coredump): %s:%u: %s\n", __FILE__, __LINE__, #cond); \
                        fatal_coredump(); \
                        for (;;); } } while (0)


// #include <assert.h>
#include <cmoc.h>

#define fflush(F) /*nothing*/

#endif

#endif
