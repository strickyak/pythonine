#ifndef PYTHONINE_READBUF_H_
#define PYTHONINE_READBUF_H_

#include "_generated_prim.h"
#include "defs.h"
#include "octet.h"
#include "standard.h"

#define READ_BUF_SIZE 128
#define OS9_EOF_ERROR 211

struct ReadBuf {
  word filename;
  byte fd;
  void* file;
  byte i;
  byte end;
  word buf;
};

void ReadBufOpen(struct ReadBuf* bp, const char* filename);

byte ReadBufCurrent(struct ReadBuf* bp);
byte ReadBufNext(struct ReadBuf* bp);
void ReadBufClose(struct ReadBuf* bp);

#endif
