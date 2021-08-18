#ifndef _PYTHONINE_DATA_H_
#define _PYTHONINE_DATA_H_

#include "chain.h"
#include "octet.h"

extern word Stdin;
extern word Stdout;
extern word Stderr;

void InitData();

// Buf

word NewBuf();
byte BufLen(word buf);
void BufAppendByte(word buf, byte b);
void BufAppendZtr(word buf, word ztr);
// BufGetZtr makes a smaller copy of a bigger buf.
word BufGetZtr(word buf);

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
word NewListIter(word base);
word ListIterNext(word it);
word NewChainIter(word base, byte cls);

// Dict
word NewDict();
byte DictWhatNth(word chain, word key);
word DictAddr(word chain, word key);
word DictGet(word chain, word key);
void DictPut(word chain, word key, word value);
word NewDictIter(word base);
word DictIterNext(word it);

// Str
byte ZtrLen(word ztr);
byte ZtrAt(word ztr, byte i);
byte ZtrAtOrZero(word ztr, byte i);

// File
byte OpenFileForReadFD(const char* filename);
word PyOpenFile(word name_str, word mode_str);
word FileReadLineToNewBuf(word file);
void SetHighBitTermination(word str);
void ClearHighBitTermination(word str);

#endif
