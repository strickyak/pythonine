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
void BufAppendStr(word buf, word ztr);
// BufGetStr makes a smaller copy of a bigger buf.
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
word NewListIter(word base);
word ListIterNext(word it);
word NewChainIter(word base, byte cls);

// Dict
word NewDict();
byte DictWhatNth(word chain, word key);
word DictAddr(word chain, word key);
word DictGet(word chain, word key);
word DictGetOrDefault(word chain, word key, word dflt);
void DictPut(word chain, word key, word value);
word NewDictIter(word base);
word DictIterNext(word it);
word DictItems(word a);

// Str
byte StrLen(word ztr);
byte StrAt(word ztr, byte i);
byte StrAtOrZero(word ztr, byte i);
word StrRStrip(word ztr);
word StrUpper(word a);
int StrCmp(word a, word b);
word StrFromInt(int x);

// File
byte OpenFileForReadFD(const char* filename);
word PyOpenFile(word name_ztr, word mode_ztr);
word FileReadLineToNewBuf(word file);
void FileWriteLine(word file, word ztr);
void OsWriteText(int fd, const char* p, byte n);

// Builtin
word BuiltinInt(word a);
word BuiltinStr(word a);
// TODO // word BuiltinRepr(word a);
word BuiltinSorted(word a);
bool LessThan(word a, word b);
word StrCat2(word a, word b);
byte ForkShellAndWait();

#endif
