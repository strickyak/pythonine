// C functions to decode our protobufs.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "octet.h"
#include "pb.h"
#include "runtime.h"

word pb_get_varint(word p, word* out) {
  word z = (ogetb(p) & 0x7F);
  while ((ogetb(p) & 0x80) == 0x80) {
    z = (z << 7) | (ogetb(++p) & 0x7F);
  }
  *out = z;
  return p + 1;
}

word pb_int(word p, word* out) {
  assert1((ogetb(p) & 7) == KIND_INT, "bad pb_int kind %d", ogetb(p) & 7);
  return pb_get_varint(p + 1, out);
}

word pb_str(word p, word* s, word* len) {
  assert1((ogetb(p) & 7) == KIND_STR, "bad pb_str kind %d", ogetb(p) & 7);
  p = pb_get_varint(p + 1, len);
  *s = p;
  return p + *len;
}
word pb_message(word p, word* message) {
  assert1((ogetb(p) & 7) == KIND_MESSAGE, "bad pb_message kind %d",
          ogetb(p) & 7);
  byte a;
  *message = ++p;
  for (a = GetB(p); a; a = GetB(p)) {
    switch (a & 7) {
      case KIND_TAG:
      case KIND_INT:
      case KIND_STR:
      case KIND_MESSAGE:
        p = pb_skip(p);
      default:
        opanic(98);
    }
  }
  return p;
}
word pb_skip(word p) {
  word _1, _2;
  switch (ogetb(p)) {
    case KIND_TAG:
      return p + 1;
    case KIND_INT: {
      return pb_int(p, &_1);
    }
    case KIND_STR: {
      return pb_str(p, &_1, &_2);
    }
    case KIND_MESSAGE: {
      return pb_message(p, &_1);
    }
    default:
      assert1(0, "bad kind %d", ogetb(p));
  }
}
