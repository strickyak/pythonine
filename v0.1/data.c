// Tuple, List, and Dict rely on Train for their implementaiton.

#include "data.h"

#include "train.h"
#include "octet.h"
#include "runtime.h"

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

// Pair
word NewPair(word first, word second) {
  word p = oalloc(4, C_Pair);
  Pair_first_Put(p, first);
  Pair_second_Put(p, second);
  return p;
}

// Buf
// Buf uses a single object of 254 Bytes.
// The first is the amount used.
// The rest are the contents.

word NewBuf() { return oalloc(254, C_Str); }

byte BufLen(word buf) { return ogetb(buf); }

void BufAppendByte(word buf, byte b) {
  byte n = ogetb(buf);
  n++;
  assert(n < 253);
  oputb(buf, n);
  oputb(buf + n, b);
  oputb(buf + n + 1, 0); // NUL termination.
}

void BufAppendStr(word buf, word str) {
  byte z_len = StrLen(str);
  word newlen = BufLen(buf) + z_len;
  assert(newlen < 253);
  oputb(buf, (byte)newlen);

  word src = str;  // for preincrement
  word dest = buf;     // for preincrement

  for (byte i = 0; i < z_len; i++) {
    oputb(++dest, ogetb(++src));
  }
  oputb(dest, 0);  // NUL termination.
}

// BufGetStr makes a smaller copy of a bigger buf.
word BufGetStr(word buf) {
  byte n = ogetb(buf);
  word str = oalloc(n+2, C_Str); // 2 = 1(len) + 1(NUL)
  oputb(str, n);  // len
  omemcpy(str+1, buf + 1, n);
  oputb(str + 1 + n, 0);  // NUL termination.
  return str;
}

// Tuple

//# word NewTuple(byte cap2) {
//#   return NewChainWithCap(Tuple_Size, C_Tuple, cap2);
//# }
word NewTuple() {
  return NewTrain(16, C_Tuple);
}
word TupleGetNth(word tuple, byte nth) { return TrainGetNth(tuple, nth); }
void TuplePutNth(word tuple, byte nth, word value) {
  TrainPutNth(tuple, nth, value);
}
void TupleAppend(word tuple, word value) { TrainAppend(tuple, value); }

// List

word NewList() { return NewTrain(16, C_List); }
int ListLen(word list) { return TrainLen2(list); }
word ListGetNth(word list, int nth) { return TrainGetNth(list, nth); }
void ListPutNth(word list, int nth, word value) {
  TrainPutNth(list, nth, value);
}
void ListAppend(word list, word value) { TrainAppend(list, value); }

// Dict
word NewDict() { return NewTrain(32, C_Dict); }
int DictLen(word dict) { return TrainLen2(dict) >> 1; }
int DictWhatNth(word chain, word key) { return TrainMapWhatNth(chain, key); }
word DictAddr(word chain, word key) { return TrainMapAddr(chain, key); }
word DictGet(word chain, word key) { return TrainMapGet(chain, key); }
word DictGetOrDefault(word chain, word key, word dflt) { return TrainMapGetOrDefault(chain, key, dflt); }
void DictPut(word chain, word key, word value) {
  return TrainMapPut(chain, key, value);
}

#if 0
word NewChainIter(word base, byte cls) {
  word it = oalloc(ListIter_Size, cls);
  ListIter_base_Put(it, base);
  ListIter_p_Put(it, List_root(base));
  ListIter_i_Put(it, 0);
  ListIter_len2_Put(it, List_len2(base));
  return it;
}
#endif
word NewListIter(word base) { return NewTrainIter(base, C_ListIter); }

word ListIterNext(word it) {
  word train = ListIter_train(it);
  assert(train);
  if (ListIter_i(it) >= Train_len2(train)) {
    train = Train_next(train);
    ListIter_train_Put(it, train);
    ListIter_i_Put(it, 0);
  }
  if (!train) {
    // Stop the iteration with an exception.
    RaiseC("StopIteration");
  }

  int j = ListIter_i(it);
  ListIter_i_Put(it, j+1);
  word addr = train + Train_Size + j + j;
  return ogetw(addr);
}

word NewDictIter(word base) { return NewTrainIter(base, C_DictIter); }

word DictIterNext(word it) {
  word key = ListIterNext(it);  // key
  (void)ListIterNext(it);       // value
  return key;
}

byte StrLen(word str) {
  CHECK(ocls(str) == C_Str, "not_str");
  return ogetb(str);  // strlen is in first byte.
}
byte StrAt(word str, byte i) {
  CHECK(ocls(str) == C_Str, "not_str");
  if (i >= StrLen(str)) RaiseC("str_ix_oob");
  return ogetb(str+1+i);
}

byte StrAtOrZero(word str, byte i) {
  CHECK(ocls(str) == C_Str, "not_str");
  if (i >= StrLen(str)) return 0;
  return ogetb(str+1+i);
}
word StrRStrip(word str) {
  // TODO -- does not seem to be a problem in OS9.
  return str;
}
word StrUpper(word a) {
  byte n = StrLen(a);
  word z = oalloc(n + 2, C_Str);
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
    pshs y,u
    lda #1 ; read mode
    ldx filename
    SWI2
    FCB $84 ; I$Open
    BCC Good.1

    lda #$FF ; INF

Good.1
    sta fd
    puls y,u
  }
#endif
  return fd;
}

word PyOpenFile(word name_str, word mode_str) {
  byte mode_c = StrAtOrZero(mode_str, 0);
  byte fd = INF;

  word p = name_str + 1;
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

void FileWriteLine(word file, word str) {
  assert(ocls(file) == C_File);
  byte fd = (byte)File_fd(file);
  // HACK -- change 10 to 13 *in place*, altering str.
  byte n = StrLen(str);
  for (byte i = 0; i < n; i++) {
    if (ogetb(str+1+i) == 10) {
      oputb(str+1+i, 13);
    }
  }
  OsWriteText(fd, (const char*) olea(str+1), n);
}
word FileReadLineToNewBuf(word file) {
  assert(ocls(file) == C_File);
  byte fd = (byte)File_fd(file);
  word buf = NewBuf();
  word ptr = buf + 1;
  byte err = 0;
  word len = 0;
  // printf(" (FRL fd=%d buf=%d) ", fd, buf);
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
  // printf(" (FRL len=%d err=%d) ", len, (int)err);
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
    case C_Str:
      x = atoi((const char*)olea(a+1));
      z = FROM_INT(x);
      break;
    default:
      RaiseC("bad_b_int");
    }
  }
  return z;
}

word StrFromInt(int x) {
  char buf[8];
  bool neg=0;
  if (x<0) {
    neg=1; x = -x;
  }
  if (x<0) RaiseC("2neg");
  byte d0 = (byte)(x % 10);
  x = x / 10;
  byte d1 = (byte)(x % 10);
  x = x / 10;
  byte d2 = (byte)(x % 10);
  x = x / 10;
  byte d3 = (byte)(x % 10);
  x = x / 10;
  byte d4 = (byte)(x % 10);

  char* p = buf;
  if (neg) *p++ = '-';
  bool show = 0;
  if (d4) *p++ = d4 + '0', show=1;
  if (show||d3) *p++ = d3 + '0', show=1;
  if (show||d2) *p++ = d2 + '0', show=1;
  if (show||d1) *p++ = d1 + '0';
  *p++ = d0 + '0';
  *p = 0;

  return StrFromC(buf);
}

word BuiltinStr(word a) {
  word z;
  byte isn;

  if (a&1) {
    // char buf[10];
    // sprintf(buf, "%d", TO_INT(a));
    // z = StrFromC(buf);
    z = StrFromInt(TO_INT(a));
  } else {
    switch(ocls(a)) {
    case C_Str:
      z = a;
      break;
    default:
      RaiseC("broken_b_str");
      isn = InternString(StrFromC("__str__"));
      PleaseCallMeth0(isn, a);
      z = None;  // special return.
    }
  }
  return z;
}

word SortedList(word a) {
  int n = ListLen(a);
  word z = NewList();

  for (int i = 0; i<n; i++) {
    ListAppend(z, ListGetNth(a, i));
  }
  if (n<2) return z;

  int m = n-1;
  for (int i = 0; i<m; i++) {
    for (int j = 0; j<m; j++) {
      word u = ListGetNth(z, j);
      word v = ListGetNth(z, j+1);
      if (Compare(u, v) > 0) {
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

cmp_t StrCmp(word a, word b) {
  byte na = StrLen(a);
  byte nb = StrLen(b);
  byte n = (na < nb) ? na : nb; // min

  for (byte i = 0; i<n; i++) {
    byte ca = StrAt(a, i);
    byte cb = StrAt(b, i);
    if (ca < cb) return -1;
    if (ca > cb) return 1;
  }
  if (na > nb) return 1;
  if (na < nb) return -1;
  return 0;
}

cmp_t CompareInt(int a, int b) {
  return (a<b)? -1 : (a>b)? +1 : 0;
}

cmp_t Compare(word a, word b) {
  if ((a&1) && (b&1)) {
    return CompareInt(TO_INT(a), TO_INT(b));
  }
  if (ocls(a)==C_Str && ocls(b)==C_Str) {
    return StrCmp(a,b);
  }
  if (ocls(a)==C_Pair && ocls(b)==C_Pair) {
    cmp_t c = Compare(Pair_first(a), Pair_first(b));
    if (c) return c;
    return Compare(Pair_second(a), Pair_second(b));
  }
  if (ocls(a)==C_Tuple && ocls(b)==C_Tuple) {
    byte u = (byte) ListLen(a);
    byte v = (byte) ListLen(b);
    byte w = (u < v) ? u : v; // min
    for (byte i = 0; i<w; i++) {
        cmp_t c = Compare(ListGetNth(a, i), ListGetNth(b, i));
        if (c!=0) return c;
    }
    return CompareInt(u, v);
  }
  return CompareInt((int)a, (int)b);
}
#if 0
extern void Show(word);
cmp_t VerboseCompare(word a, word b) {
  bool z = Compare(a, b);
  printf("Compare %d <- ((", z);
  Show(a);
  Show(b);
  printf("))\n");
  return z;
}
#endif

word StrCat2(word a, word b) {
  byte na = StrLen(a);
  byte nb = StrLen(b);
  int nn = na + nb;
  CHECK(nn<253, "strcat2_too_big");
  byte n = (byte)nn;
  word z = oalloc(n+2, C_Str);
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
  LoopAllocRoot = z; // Protect tuple allocs inside this list.
  // printf("\n==DictItems== a=%d z=%d\n", a, z);
  struct TrainIterator it;

  TrainIterStart(a, &it);
  while (TrainIterMore(&it)) {
    word key = TrainIterNext(&it);
    word val = TrainIterNext(&it);
    ListAppend(z, NewPair(key, val));
  }
  LoopAllocRoot = None;
  return z;
}

byte ForkShellAndWait() {
#if unix
  byte err = (byte) system("/bin/sh");
#else
  const char* name = "SHELL";
  byte pid;
  byte err=0;
  asm {
    daa
    daa
    daa
    daa
    pshs y,u 

    clra       ; any lang
    clrb       ; D=0
    ldx name  ; SHELL name
    tfr d,y       ; no params
    tfr d,u       ; no param memory
    ldb #32    ; data pages

    SWI2
    FCB $03 ; F$Fork
    puls y,u
    BCS Bad_fs

    sta pid
    pshs y,u 

Again_fs
    SWI2
    FCB $04 ; F$Wait
    puls y,u
    BCS Bad_fs

    cmpa pid  ; did the right child die?
    BNE Again_fs
* Fall thru, storing B to err.

Bad_fs
    STB err
  }
#endif
  return err;
}
