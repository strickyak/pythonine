// Tuple, List, and Dict rely on Chain for their implementaiton.

#include "data.h"

#include "chain.h"
#include "octet.h"

word Stdin;
word Stdout;
word Stderr;
#if unix
FILE* FileHandles[100];
int NextFileHandle;
#endif

void InitData() {
  Stdin = oalloc(File_Size, C_File);
  File_fd_Put(Stdin, 0);
  Stdout = oalloc(File_Size, C_File);
  File_fd_Put(Stdout, 1);
  Stderr = oalloc(File_Size, C_File);
  File_fd_Put(Stderr, 2);
#if unix
  FileHandles[0] = stdin;
  FileHandles[1] = stdout;
  FileHandles[2] = stderr;
  NextFileHandle = 3;
#endif
}

// Buf
// Buf uses a single object of 254 Bytes.
// The first is the amount used.
// The rest are the contents.

word NewBuf() { return oalloc(254, C_Ztr); }

byte BufLen(word buf) { return ogetb(buf); }

void BufAppendByte(word buf, byte b) {
  byte n = ogetb(buf);
  n++;
  assert(n < 253);
  oputb(buf, n);
  oputb(buf + n, b);
  oputb(buf + n + 1, 0); // NUL termination.
}

void BufAppendZtr(word buf, word ztr) {
  byte z_len = ZtrLen(ztr);
  word newlen = BufLen(buf) + z_len;
  assert(newlen < 253);
  oputb(buf, (byte)newlen);

  word src = ztr;  // for preincrement
  word dest = buf;     // for preincrement

  for (byte i = 0; i < z_len; i++) {
    oputb(++dest, ogetb(++src));
  }
  oputb(dest, 0);  // NUL termination.
}

// BufGetZtr makes a smaller copy of a bigger buf.
word BufGetZtr(word buf) {
  byte n = ogetb(buf);
  word ztr = oalloc(n+2, C_Ztr); // 2 = 1(len) + 1(NUL)
  oputb(ztr, n);  // len
  omemcpy(ztr+1, buf + 1, n);
  oputb(ztr + 1 + n, 0);  // NUL termination.
  return ztr;
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
word DictGetOrDefault(word chain, word key, word dflt) { return ChainMapGetOrDefault(chain, key, dflt); }
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
    RaiseC("StopIteration");
  }
  ListIter_len2_Put(it, len2 - 1);

  word p = ListIter_p(it);
  int i = ListIter_i(it);
  word z = ogetw(p + i);
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

byte ZtrLen(word ztr) {
  CHECK(ocls(ztr) == C_Ztr, "not_ztr");
  return ogetb(ztr);  // ztrlen is in first byte.
}
byte ZtrAt(word ztr, byte i) {
  CHECK(ocls(ztr) == C_Ztr, "not_ztr");
  if (i >= ZtrLen(ztr)) RaiseC("ztr_ix_oob");
  return ogetb(ztr+1+i);
}

byte ZtrAtOrZero(word ztr, byte i) {
  CHECK(ocls(ztr) == C_Ztr, "not_ztr");
  if (i >= ZtrLen(ztr)) return 0;
  return ogetb(ztr+1+i);
}
word ZtrRStrip(word ztr) {
  // TODO -- does not seem to be a problem in OS9.
  return ztr;
}
word ZtrUpper(word a) {
  byte n = ZtrLen(a);
  word z = oalloc(n + 2, C_Ztr);
  oputb(z, n);
  oputb(z+1+n, 0);
  for (byte i = 0; i < n; i++) {
    byte c = ogetb(a+1+i);
    if ('a' <= c && c <= 'z') {
      oputb(z+1+i, c-32);
    } else {
      oputb(z+1+i, c);
    }
  }
  return z;
}

byte OpenFileForReadFD(const char* filename) {
  byte fd;
#if unix
  char unixname[252];
  memset(unixname, 0, sizeof unixname);
  for (int i = 0; filename[i]; i++) {
    unixname[i] = filename[i] & 127;
    if (filename[i] & 128) {
      break;
    }
  }
  FILE* f = fopen(unixname, "r");
  if (!f) RaiseC("cant_open");
  FileHandles[NextFileHandle] = f;
  fd = (byte)NextFileHandle;
  assert(fd < 100);
  ++NextFileHandle;
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

word PyOpenFile(word name_ztr, word mode_ztr) {
  byte mode_c = ZtrAtOrZero(mode_ztr, 0);
  byte fd = INF;

  word p = name_ztr + 1;
  switch (mode_c) {
    case 'r':
      fd = OpenFileForReadFD((const char*)olea(p));
      break;
    case 'w':
      RaiseC("todo_w");
    default:
      RaiseC("bad_mode");
  }
  CHECK(fd != INF, "cant_open");

  word file = oalloc(File_Size, C_File);
  File_fd_Put(file, fd);
  return file;
}

void OsWriteText(int fd, const char* p, byte n) {
  byte err = 0;
#if unix
  int ok = fputs(p, FileHandles[fd]);
  if (ok < 0) {
    RaiseC("bad_fputs");
  }
#else
  asm {
      PSHS Y,U

      CLRA
      LDB n
      TFR D,Y  # number bytes

      LDA fd
      LDX p    # pointer

      SWI2
      FCB 0x8C  # I$WritLn
      BCC OWT_ok

      STB err

OWT_ok
      PULS Y,U
  }
#endif
  if (err) RaiseC("bad_wr");
}

void FileWriteLine(word file, word ztr) {
  assert(ocls(file) == C_File);
  byte fd = (byte)File_fd(file);
  // HACK -- change 10 to 13 *in place*, altering ztr.
  byte n = ZtrLen(ztr);
  for (byte i = 0; i < n; i++) {
    if (ogetb(ztr+1+i) == 10) {
      oputb(ztr+1+i, 13);
    }
  }
  OsWriteText(fd, (const char*) olea(ztr+1), n);
}
word FileReadLineToNewBuf(word file) {
  assert(ocls(file) == C_File);
  byte fd = (byte)File_fd(file);
  word buf = NewBuf();
  word ptr = buf + 1;
  byte err = 0;
  word len = 0;
  // printf("FRL fd=%d buf=%d\n", fd, buf);
#if unix
  char* zz = fgets((char*)olea(ptr), 252, FileHandles[fd]);
  if (!zz)
    err = 1;
  else
    len = (word)strlen(zz);
#else
  asm {
      pshs y,u
      lda fd
      ldx ptr
      ldy #252
      SWI2
      FCB $8B ; I$ReadLn
      BCC Good.2

      stb err
Good.2
      sty len
      puls y,u
  }
#endif
  // printf("FRL len=%d err=%d\n", len, (int)err);
  assert(len >= 0);
  assert(len <= 252);
  if (err || !len) {
    ofree(buf);
    return None;
  }
  byte last = ogetb(buf + len /*+1-1*/);
  if (0 < last && last < 32) {
    oputb(buf + len /*+1-1*/, 0);  // replace CR/LF with NUL.
    --len;
  }
  oputb(buf, len);  // len goes in slot 0.
  return buf;
}

word BuiltinInt(word a) {
  word z;
  int x;
  if (a&1) {
    z = a;
  } else {
    switch(ocls(a)) {
    case C_Ztr:
      x = atoi((const char*)olea(a+1));
      z = FROM_INT(x);
      break;
    default:
      RaiseC("bad_b_int");
    }
  }
  return z;
}

word ZtrFromInt(int x) {
  char buf[8];
  bool neg=0;
  if (x<0) {
    neg=1; x = -x;
  }
  if (x<0) RaiseC("2neg");
  int d0 = x % 10;
  x = x / 10;
  int d1 = x % 10;
  x = x / 10;
  int d2 = x % 10;
  x = x / 10;
  int d3 = x % 10;
  x = x / 10;
  int d4 = x % 10;

  char* p = buf;
  if (neg) *p++ = '-';
  bool show = 0;
  if (d4) *p++ = d4 + '0', show=1;
  if (show||d3) *p++ = d3 + '0', show=1;
  if (show||d2) *p++ = d2 + '0', show=1;
  if (show||d1) *p++ = d1 + '0';
  *p++ = d0 + '0';
  *p = 0;

  return ZtrFromC(buf);
}

word BuiltinStr(word a) {
  word z;
  byte isn;

  if (a&1) {
    // char buf[10];
    // sprintf(buf, "%d", TO_INT(a));
    // z = ZtrFromC(buf);
    z = ZtrFromInt(TO_INT(a));
  } else {
    switch(ocls(a)) {
    case C_Ztr:
      z = a;
      break;
    default:
      RaiseC("broken_b_str");
      isn = InternZtring(ZtrFromC("__str__"));
      PleaseCallMeth0(isn, a);
      z = None;  // special return.
    }
  }
  return z;
}

word SortedList(word a) {
  byte n = List_len2(a);
  word z = NewList();

  for (byte i = 0; i<n; i++) {
    ListAppend(z, ListGetNth(a, i));
  }
  if (n<2) return z;

  byte m = n-1;
  for (byte i = 0; i<m; i++) {
    for (byte j = 0; j<m; j++) {
      word u = ListGetNth(z, j);
      word v = ListGetNth(z, j+1);
      if (v < u) {
        ListPutNth(z, j, v);
        ListPutNth(z, j+1, u);
      }
    }
  }
  return z;
}
word BuiltinSorted(word a) {
  word z;
  if (a&1) {
      RaiseC("bad_b_sorted");
  } else {
    switch(ocls(a)) {
    case C_List:
      z = SortedList(a);
      break;
    default:
      RaiseC("bad_b_sorted");
    }
  }
  return z;
}

int ZtrCmp(word a, word b) {
  byte na = ZtrLen(a);
  byte nb = ZtrLen(b);
  byte n = (na < nb) ? na : nb;

  for (byte i = 0; i<n; i++) {
    byte ca = ZtrAt(a, i);
    byte cb = ZtrAt(b, i);
    if (ca < cb) return -1;
    if (ca > cb) return 1;
  }
  if (na > nb) return 1;
  if (na < nb) return -1;
  return 0;
}

bool LessThan(word a, word b) {
  if ((a&1) && (b&1)) {
    return TO_INT(a) < TO_INT(b);
  }
  if (ocls(a)==C_Ztr && ocls(b)==C_Ztr) {
    return ZtrCmp(a,b) < 0;
  }
  return a < b;
}

word ZtrCat2(word a, word b) {
  byte na = ZtrLen(a);
  byte nb = ZtrLen(b);
  int nn = na + nb;
  CHECK(nn<253, "ztrcat2_too_big");
  byte n = (byte)nn;
  word z = oalloc(n+2, C_Ztr);
  oputb(z, n);
  oputb(z+1+n, 0);
  for (int i=0; i<na; i++) {
    oputb(z+1+i, ogetb(a+1+i));
  }
  for (int i=0; i<nb; i++) {
    oputb(z+1+na+i, ogetb(b+1+i));
  }
  return z;
}

word DictItems(word a) {
  word z = NewList();
  struct ChainIterator it;

  ChainIterStart(a, &it);
  while (ChainIterMore(a, &it)) {
    word key = ChainIterNext(a, &it);
    word val = ChainIterNext(a, &it);
    word tup = NewTuple();
    TupleAppend(tup, key);
    TupleAppend(tup, val);
    ListAppend(z, tup);
  }
  return z;
}
