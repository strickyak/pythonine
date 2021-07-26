#ifndef PYTHONINE_STANDARD_H_
#define PYTHONINE_STANDARD_H_

#ifdef unix

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#else

#include <assert.h>
#include <cmoc.h>

#define fflush(F) /*nothing*/

#endif

#endif
