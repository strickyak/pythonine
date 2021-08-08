// Tuple, List, and Dict rely on Chain for their implementaiton.

#include "data.h"

#include "chain.h"
#include "octet.h"

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
