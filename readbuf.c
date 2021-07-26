#include "readbuf.h"
#include "pb2.h"

void ReadBufChunk(struct ReadBuf* bp) {
  bp->i = 0;
#if unix
  bp->end = (byte)fread(olea(bp->buf), 1, READ_BUF_SIZE, bp->file);
#else
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
    // printf(" [read got OS9_EOF]\n");
    bp->end = 0;
  }
  else {
    assert2(!err, "reading %s err %d", olea(bp->filename), (word)err);
    bp->end = (byte)bytes_read;
  }
#endif
  // printf("ReadBufChunk: bytes_read=%d cap=%d\n", (word)bp->end,
  // ocap(bp->buf)); for (byte i = 0; i < bp->end; i++) { printf(" %02x",
  // ogetb(bp->buf + i));
  // }
}

void ReadBufOpen(struct ReadBuf* bp, const char* filename) {
  assert(bp);
  int fn_size = strlen(filename) + 1;
  assert(fn_size < INF);
  bp->filename = oalloc((byte)fn_size, BYTES_CLASS);
  memcpy(olea(bp->filename), filename, fn_size);
  bp->buf = oalloc(READ_BUF_SIZE, BYTES_CLASS);
  // printf("cap(bp->buf) = %d\n", ocap(bp->buf));
#if unix
  bp->fd = INF;
  bp->file = fopen(filename, "r");
  assert2((long)bp->file, "opening %s err %d", filename, errno);
#else
  bp->file = NULL;
  byte fd = INF;
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
  assert2(!err, "opening %s err %d", olea(filename), (word)err);
  bp->fd = fd;
#endif
  ReadBufChunk(bp);
}

byte ReadBufCurrent(struct ReadBuf* bp) {
  if (!bp->end) return 0;  // EOF.
  return ogetb(bp->buf + bp->i);
  // printf(" (%02x) ", z);
}
byte ReadBufNext(struct ReadBuf* bp) {
  if (!bp->end) goto on_eof;
  ++bp->i;
  if (bp->i >= bp->end) {
    ReadBufChunk(bp);
    if (!bp->end) goto on_eof;
  }
  return ogetb(bp->buf + bp->i);
  // printf(" <-%02x \n", z);

on_eof:
  // printf(" <-EOF \n");
  return 0;
}
void ReadBufClose(struct ReadBuf* bp) {
#if unix
  fclose(bp->file);
#else
  byte fd = bp->fd;
  asm {
            PSHS Y,U
            LDA fd
            SWI2
            FCB $8F     ; I$Close
            PULS Y,U
  }
#endif
}
