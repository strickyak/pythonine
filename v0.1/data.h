#ifndef _PYTHONINE_DATA_H_
#define _PYTHONINE_DATA_H_

#include "train.h"
#include "octet.h"

typedef signed char cmp_t;

extern word Stdin;
extern word Stdout;
extern word Stderr;

void InitData();

// Pair
word NewPair(word first, word second);

// Buf

word NewBuf();
byte BufLen(word buf);
void BufAppendByte(word buf, byte b);
void BufAppendStr(word buf, word str);
// BufGetStr makes a smaller copy of a bigger buf.
word BufGetStr(word buf);

// Tuple
word NewTuple();
word TupleGetNth(word tup, byte nth);
void TuplePutNth(word tup, byte nth, word value);
void TupleAppend(word tup, word value);

// List
word NewList();
int ListLen(word list);
word ListGetNth(word list, int nth);
void ListPutNth(word list, int nth, word value);
void ListAppend(word list, word value);
word NewListIter(word base);
word ListIterNext(word it);
word NewChainIter(word base, byte cls);

// Dict
word NewDict();
int DictLen(word dict);
int DictWhatNth(word chain, word key);
word DictAddr(word chain, word key);
word DictGet(word chain, word key);
word DictGetOrDefault(word chain, word key, word dflt);
void DictPut(word chain, word key, word value);
word NewDictIter(word base);
word DictIterNext(word it);
word DictItems(word a);

// Str
byte StrLen(word str);
byte StrAt(word str, byte i);
byte StrAtOrZero(word str, byte i);
word StrRStrip(word str);
word StrUpper(word a);
cmp_t StrCmp(word a, word b);
word StrFromInt(int x);

// File
byte OpenFileForReadFD(const char* filename);
word PyOpenFile(word name_str, word mode_str);
word FileReadLineToNewBuf(word file);
void FileWriteLine(word file, word str);
void OsWriteText(int fd, const char* p, byte n);

// Builtin
cmp_t CompareInt(int a, int b);
cmp_t Compare(word a, word b);

word BuiltinInt(word a);
word BuiltinStr(word a);
// TODO // word BuiltinRepr(word a);
word BuiltinSorted(word a);
bool LessThan(word a, word b);
word StrCat2(word a, word b);
byte ForkShellAndWait();

#endif
