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
