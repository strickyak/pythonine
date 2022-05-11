#ifndef _RAWIO_H_
#define _RAWIO_H_

#include "octet.h"
#include "standard.h"

// All return 0 or error code.
byte RawOpen(const char* name, byte* fd_out);
byte RawRead(byte fd, byte* ptr, word n, word* out_read);
byte RawCreate(const char* name, byte* fd_out);
byte RawWrite(byte fd, byte* ptr, word n, word* out_written);  // can short write.
byte RawClose(byte fd);

byte RawRead1(byte fd, byte* out_b);  // read one byte.
byte RawWrite1(byte fd, byte b);  // write one byte.

#endif // _RAWIO_H_
