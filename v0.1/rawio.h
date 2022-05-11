#ifndef _RAWIO_H_
#define _RAWIO_H_

#include "octet.h"
#include "standard.h"

// All return 0 or error code.
byte RawCreate(const char* name, byte* fd_out);
byte RawWrite(byte fd, byte* ptr, word n, word* out_written);  // can short write.
byte RawClose(byte fd);

byte RawWrite1(byte fd, byte b);  // for one byte.

#endif // _RAWIO_H_
