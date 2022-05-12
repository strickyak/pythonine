#ifndef PYTHONINE_READBUF_H_
#define PYTHONINE_READBUF_H_

#include "ur.h"

#define READ_BUF_SIZE 128
#define OS9_EOF_ERROR 211

struct ReadBuf {
  byte fd;
  byte i;
  byte end;
  char* buf;
};

void ReadBufOpen(struct ReadBuf* bp, const char* filename, char* buffer);

byte ReadBufCurrent(struct ReadBuf* bp);
byte ReadBufNext(struct ReadBuf* bp);
void ReadBufClose(struct ReadBuf* bp);

#endif
