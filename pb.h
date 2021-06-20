// C functions to decode our protobufs.

#ifndef PYTHONINE_PB_H_
#define PYTHONINE_PB_H_

#include "octet.h"

// The low 3 bits in a ttag (the upper 5 bits
// will be the tag number defined in the .proto file).
enum PbKindTag {
  KIND_TAG = 0,
  KIND_INT = 1,
  KIND_STR = 2,
  KIND_MESSAGE = 3,
};

// Checks the tag, sets *out, and returns the new p.
extern word pb_int(word p, word* out);
// Checks the tag, sets *s and *len, and returns the new p.
extern word pb_str(word p, word* s, word* len);
// Checks the tag, sets *message to the next address, and advances over the
// message.
extern word pb_message(word p, word* message);
// Advances over the current tag.
extern word pb_skip(word p);
// Reads a varint, least significant bits first,
// reading more bits while high bit is set,
// and returns the address following the varint.
extern word pb_get_varint(word p, word* out);

#endif
