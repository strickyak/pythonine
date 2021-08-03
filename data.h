#ifndef _PYTHONINE_DATA_H_
#define _PYTHONINE_DATA_H_

#include "chain.h"

extern word NewBuf();
extern byte BufLen(word buf);
extern void BufAppendByte(word buf, byte b);
extern word BufGetStr(word buf);

#endif
