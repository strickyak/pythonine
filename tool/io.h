#ifndef _IO_H_
#define _IO_H_

#include "types.h"

error FOpen(const char* filename, bool to_write, byte* fd_out);
error FRead(byte fd, char* buf, word buf_size, word* bytes_read_out);
void FClose(int fd);

#endif
