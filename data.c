// Tuple, List, and Dict rely on Chain for their implementaiton.

#include "data.h"

#include "chain.h"
#include "octet.h"

word Stdin;
word Stdout;
word Stderr;

void InitData() {
  Stdin = oalloc(File_Size, C_File);
  File_fd_Put(Stdin, FROM_INT(0));
  Stdout = oalloc(File_Size, C_File);
  File_fd_Put(Stdout, FROM_INT(1));
  Stderr = oalloc(File_Size, C_File);
  File_fd_Put(Stderr, FROM_INT(2));
}

// Buf
// Buf uses a single object of 254 Bytes.
// The first is the amount used.
// The rest are the contents.

word NewBuf() { return oalloc(254, C_Buf); }

byte BufLen(word buf) { return ogetb(buf); }

void BufAppendByte(word buf, byte b) {
  byte n = ogetb(buf);
  n++;
  assert(n < 254);
  oputb(buf, n);
  oputb(buf + n, b);
}

void BufAppendStr(word buf, word str) {
  byte s_len = (byte)Str_len(str);
  word newlen = BufLen(buf) + s_len;
  assert(newlen < 254);
  oputb(buf, (byte)newlen);

  word src =
      Str_bytes(str) + (byte)Str_offset(str) - 1;  // -1 for for preincrement.
  word dest = buf;                                 // for preincrement

  for (byte i = 0; i < s_len; i++) {
    oputb(++dest, ogetb(++src));
  }
}

word BufGetStr(word buf) {
  byte n = ogetb(buf);
  word guts = oalloc(n, C_Bytes);
  omemcpy(guts, buf + 1, n);
  word str = oalloc(Str_Size, C_Str);
  Str_bytes_Put(str, guts);
  Str_len_Put(str, n);
  Str_offset_Put(str, 0);
  return str;
}

// Tuple

word NewTuple() { return NewChain(Tuple_Size, C_Tuple); }
word TupleGetNth(word tuple, byte nth) { return ChainGetNth(tuple, nth); }
void TuplePutNth(word tuple, byte nth, word value) {
  ChainPutNth(tuple, nth, value);
}
void TupleAppend(word tuple, word value) { ChainAppend(tuple, value); }

// List

word NewList() { return NewChain(List_Size, C_List); }
word ListGetNth(word list, byte nth) { return ChainGetNth(list, nth); }
void ListPutNth(word list, byte nth, word value) {
  ChainPutNth(list, nth, value);
}
void ListAppend(word list, word value) { ChainAppend(list, value); }

// Dict
word NewDict() { return NewChain(Dict_Size, C_Dict); }
byte DictWhatNth(word chain, word key) { return ChainMapWhatNth(chain, key); }
word DictAddr(word chain, word key) { return ChainMapAddr(chain, key); }
word DictGet(word chain, word key) { return ChainMapGet(chain, key); }
void DictPut(word chain, word key, word value) {
  return ChainMapPut(chain, key, value);
}

word NewChainIter(word base, byte cls) {
  word it = oalloc(ListIter_Size, cls);
  ListIter_base_Put(it, base);
  ListIter_p_Put(it, List_root(base));
  ListIter_i_Put(it, 0);
  ListIter_len2_Put(it, List_len2(base));
  return it;
}
word NewListIter(word base) { return NewChainIter(base, C_ListIter); }

word ListIterNext(word it) {
  int len2 = ListIter_len2(it);
  if (!len2) {
    // Stop the iteration with an exception.
    RaiseC("StopIter");
  }
  ListIter_len2_Put(it, len2 - 1);

  word p = ListIter_p(it);
  int i = ListIter_i(it);
  word z = ogetw(p + i);
  // printf("ListIterNext: len2=%d p=%d cap=%d i=%d z=%d\n", len2, p, ocap(p),
  // i, z);
  i += 2;

  byte cap = ocap(p);
  if (i == cap - 2) {
    // overflow
    ListIter_p_Put(it, ogetw(ListIter_p(it) + cap - 2));
    ListIter_i_Put(it, 0);
  } else {
    // normal advance
    ListIter_i_Put(it, i);
  }
  return z;
}

word NewDictIter(word base) { return NewChainIter(base, C_DictIter); }

word DictIterNext(word it) {
  word key = ListIterNext(it);  // key
  (void)ListIterNext(it);       // value
  return key;
}

byte StrGet(word str, byte i) {
  CHECK(ocls(str) == C_Str, "not_str");
  CHECK(i < Str_len(str), "str_ix");
  word p = Str_bytes(str) + Str_offset(str) + i;
  return ogetb(p);
}

void SetHighBitTermination(word str) {
  CHECK(ocls(str) == C_Str, "not_str");
  CHECK(Str_len(str), "not_len");
  word p = Str_bytes(str) + Str_offset(str) + Str_len(str) - 1;
  byte last = ogetb(p);
  oputb(p, 0x80 | last);
}
void ClearHighBitTermination(word str) {
  CHECK(ocls(str) == C_Str, "not_str");
  CHECK(Str_len(str), "not_len");
  word p = Str_bytes(str) + Str_offset(str) + Str_len(str) - 1;
  byte last = ogetb(p);
  oputb(p, 0x7F & last);
}

#if unix
FILE* FileHandles[100];
int NextFileHandle;
#endif

byte OpenFileForReadFD(const char* filename) {
  byte fd;
#if unix
  char unixname[250];
  for (int i = 0; filename[i]; i++) {
    unixname[i] = filename[i] & 127;
    if (filename[i] & 128) break;
  }
  FILE* f = fopen(unixname, "r");
  FileHandles[NextFileHandle] = f;
  fd = (byte)NextFileHandle++;
  assert(fd < 100);
#else
  asm {
    lda #1 ; read mode
    ldx filename
    SWI2
    FCB $84 ; I$Open
    BCC Good.1

    lda #$FF ; INF

Good.1
    sta fd
  }
#endif
  return fd;
}

word PyOpenFile(word name_str, word mode_str) {
  byte mode_c = StrGet(mode_str, 0);
  byte fd = INF;

  word p = Str_bytes(name_str) + Str_offset(name_str);
  SetHighBitTermination(name_str);
  switch (mode_c) {
    case 'r':
      fd = OpenFileForReadFD((const char*)olea(p));
      break;
    case 'w':
      RaiseC("todo_w");
    default:
      RaiseC("bad_mode");
  }
  ClearHighBitTermination(name_str);
  CHECK(fd != INF, "cant_open");

  word file = oalloc(File_Size, C_File);
  File_fd_Put(file, fd);
  return file;
}

word FileReadLineToBuf(word file) {
  byte fd = (byte)File_fd(file);
  word buf = NewBuf();
  word ptr = buf + 1;
  byte err = 0;
  word len = 0;
#if unix
  char* zz = fgets((char*)olea(ptr), 250, FileHandles[fd]);
  if (!zz)
    err = 1;
  else
    len = (word)strlen(zz);
#else
  asm {
      pshs y,u
      lda fd
      ldx ptr
      ldy #250
      SWI2
      FCB $8B ; I$ReadLn
      BCC Good.2

      stb err
Good.2
      sty len
      puls y,u
  }
#endif
  if (err || !len) {
    ofree(buf);
    return None;
  }
  byte last = ogetb(buf + len /*+1-1*/);
  if (0 < last && last < 32) {
    oputb(buf + len /*+1-1*/, 0);  // replace CR/LF with NUL.
    --len;
  }
  oputb(buf, len);                      // len goes in slot 0.
  printf("BUF: %d: $s\n", ogetb(buf));  // $ DIS: olea(buf+1)
  return buf;
}
