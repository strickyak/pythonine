#include "readbuf.h"

// #include "pb2.h"

#define debugf if(0)printf

void ReadBufChunk(struct ReadBuf* bp) {
  bp->i = 0;

  byte fd = bp->fd;
  word buf = bp->buf;
  byte err = 0;
  word bytes_read = 0;
  word buf_size = READ_BUF_SIZE;
  asm {
            PSHS Y,U
            LDA fd
            LDX buf
            LDY buf_size

            SWI2
            FCB $89   ; I$Read

            TFR Y,X   ; bytes_read to X
            PULS Y,U
            BCS read_err
            STX bytes_read
            BRA read_end
read_err:
            STB err
read_end:
  }
  if (err == OS9_EOF_ERROR) {
    debugf(" [read got OS9_EOF]\n");
    bp->end = 0;
  } else {
    checkerr(err);
    bp->end = (byte)bytes_read;
  }
}

void ReadBufOpen(struct ReadBuf* bp, const char* filename, char* buffer) {
  debugf("ReadBufOpen: filename=%s\n", filename);
  assert(bp);

  bp->buf = buffer;
  bp->i = 0;
  bp->end = 0;
  byte fd = 255;
  byte err = 0;
  asm {
            PSHS Y,U
            LDX filename
            LDA #1   ; read only

            SWI2
            FCB $84  ; I$Open
            PULS Y,U
            BCS open_err
            STA fd
            BRA open_end
open_err:
            STB err
open_end:
  }
  checkerr(err);
  bp->fd = fd;

  ReadBufChunk(bp);
}

byte ReadBufCurrent(struct ReadBuf* bp) {
  byte z;
  if (!bp->end) {
    z = 0;  // EOF.
  } else { 
    z = bp->buf[bp->i];
  }
  // debugf(" (%02x) ", z);
  return z;
}
byte ReadBufNext(struct ReadBuf* bp) {
  if (!bp->end) goto on_eof;
  ++bp->i;
  if (bp->i >= bp->end) {
    ReadBufChunk(bp);
    if (!bp->end) goto on_eof;
  }
  return bp->buf [ bp->i];
  // printf(" <-%02x \n", z);

on_eof:
  // printf(" <-EOF \n");
  return 0;
}
void ReadBufClose(struct ReadBuf* bp) {
  byte fd = bp->fd;
  asm {
            PSHS Y,U
            LDA fd
            SWI2
            FCB $8F     ; I$Close
            PULS Y,U
  }
}
