#ifndef _PYTHONINE_DATA_H_
#define _PYTHONINE_DATA_H_

#include "chain.h"
#include "octet.h"

// Buf

word NewBuf();
byte BufLen(word buf);
void BufAppendByte(word buf, byte b);
void BufAppendStr(word buf, word str);
word BufGetStr(word buf);

// Tuple
word NewTuple();
word TupleGetNth(word tup, byte nth);
void TuplePutNth(word tup, byte nth, word value);
void TupleAppend(word tup, word value);

// List
word NewList();
word ListGetNth(word list, byte nth);
void ListPutNth(word list, byte nth, word value);
void ListAppend(word list, word value);

// Dict
word NewDict();
byte DictWhatNth(word chain, word key);
word DictAddr(word chain, word key);
word DictGet(word chain, word key);
void DictPut(word chain, word key, word value);

#endif
