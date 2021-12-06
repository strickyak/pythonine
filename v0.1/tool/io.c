#include "io.h"

#define I_Open 0x84

#if unix
bool FileHandles[100];
#endif

error FOpen(const char* filename, bool to_write, byte* fd_out) {
  byte fd = 255;
  error err = 0;
#if unix
  FILE* f = fopen(unixname, mode);
  if (!f) RaiseC("cant_open");
  FileHandles[NextFileHandle] = f;
  fd = (byte)NextFileHandle;
  assert(fd < 100);
  ++NextFileHandle;
#else
  byte os9mode = to_write ? 2 : 1;
  asm {
    pshs y,u
    lda os9mode
    ldx filename
    SWI2
    FCB I_Open
    puls y,u
    BCC OSO_GOOD

    stb err
    bra OSO_END

OSO_GOOD
    sta fd
OSO_END
  }
#endif
  *fd_out = fd;
  return err;
}

byte FRead(byte fd, char* buf, word buf_size, word* bytes_read_out) {
#if unix
  int bytes_read = fread(FileHandles[fd], 1, READ_BUF_SIZE, bp->file);
  if (bytes_read_out) *bytes_read_out = bytes_read;
  return ferror(fd);
#else
  byte err = 0;
  word bytes_read = 0;
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
  if (bytes_read_out) *bytes_read_out = bytes_read;
  return err;
#endif
}

void FClose(int fd) {
#if unix
  fclose(FileHandle[fd]);
#else
  asm {
            PSHS Y,U
            LDA fd
            SWI2
            FCB $8F     ; I$Close
            PULS Y,U
  }
#endif
}
