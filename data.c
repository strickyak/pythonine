#include "data.h"

#include "chain.h"

word NewBuf() { return oalloc(254, C_Buf); }

byte BufLen(word buf) { return ogetb(buf); }

void BufAppendByte(word buf, byte b) {
  byte n = ogetb(buf);
  n++;
  assert(n < 254);
  oputb(buf, n);
  oputb(buf + n, b);
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
