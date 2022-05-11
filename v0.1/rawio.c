#include "rawio.h"

#include "os9.h"
#include "os9errno.h"
#include "octet.h"
#include "standard.h"
#include "_generated_prim.h"

#if unix
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <unistd.h>

byte RawOpen(const char* name, byte* fd_out) {
  // int open(const char *pathname, int flags);
  int fd = open(name, O_RDONLY);
  if (fd<0) {
    *fd_out = INF;
    return errno? (byte)errno: (byte)255;
  }
  *fd_out = fd;
  return 0;
}

byte RawCreate(const char* name, byte* fd_out) {
  // int creat(const char *pathname, mode_t mode);
  int fd = creat(name, 0644);
  if (fd<0) {
    *fd_out = INF;
    return errno? (byte)errno: (byte)255;
  }
  *fd_out = fd;
  return 0;
}

byte RawRead(byte fd, byte* ptr, word n, word* out_read) {
  // ssize_t read(int fd, void *buf, size_t count);
  ssize_t cc = read(fd, (void*) ptr, (size_t) n);
  out_read = 0;
  if (cc<0) {
    return errno? (byte)errno: (byte)255;
  } else if (cc==0) {
    // Unix does not use an error code for EOF.  OS-9 does.
    return E_EOF;
  }
  *out_read = (word)cc;
  return 0;
}

byte RawWrite(byte fd, byte* ptr, word n, word* out_written) {
  // ssize_t write(int fd, const void *buf, size_t count);
  ssize_t cc = write(fd, (const void*) ptr, (size_t) n);
  out_written = 0;
  if (cc<=0) {
    return errno? (byte)errno: (byte)255;
  } 
  *out_written = (word)cc;
  return 0;
}

byte RawClose(byte fd) {
  // int close(int fd);
  int e = close(fd);
  if (e<0) {
    return errno? (byte)errno: (byte)255;
  }
  return 0;
}

#else

byte RawOpen(const char* name, byte* fd_out) {
  byte fd = INF;
  byte err = 0;
  asm {
    lda #$01     ; A: read.
    ldx name
    pshs y,u
    swi2
    fcb I_OPEN
    puls y,u
    bcc RawOpenOK
    stb err
RawOpenOK
    sta fd
  }
  printf(" ROpen->%d,%d. ", fd, err);
  *fd_out = fd;
  return err;
}

byte RawCreate(const char* name, byte* fd_out) {
  byte fd = INF;
  byte err = 0;
  asm {
    ldd #$0203     ; A: write ; B: read|write
    ldx name
    pshs y,u
    swi2
    fcb I_CREATE
    puls y,u
    bcc RawCreateOK
    stb err
RawCreateOK
    sta fd
  }
  *fd_out = fd;
  return err;
}

byte RawRead(byte fd, byte* ptr, word n, word* out_read) {
  word bytes_read = 0;
  byte err = 0;
  asm {
    pshs y,u  ; before ldy, but C-frame U will be used.

    lda fd
    ldx ptr
    ldy n

    swi2
    fcb I_READ
    tfr y,x     # number bytes read
    puls y,u

    bcc RawReadOK
    stb err
RawReadOK
    stx bytes_read
  }
  *out_read = bytes_read;
  return err;
}

byte RawWrite(byte fd, byte* ptr, word n, word* out_written) {
  word bytes_written = 0;
  byte err = 0;
  asm {
    pshs y,u  ; before ldy, but C-frame U will be used.

    lda fd
    ldx ptr
    ldy n

    swi2
    fcb I_WRITE
    tfr y,x     # number bytes written
    puls y,u

    bcc RawWriteOK
    stb err
RawWriteOK
    stx bytes_written
  }
  *out_written = bytes_written;
  return err;
}

byte RawClose(byte fd) {
  byte err = 0;
  asm {
    lda fd
    pshs y,u
    swi2
    fcb  I_CLOSE
    puls y,u
    bcc RawCloseOK
    stb err
RawCloseOK
  }
  return err;
}
#endif

byte RawRead1(byte fd, byte* out_b) {
  word cc;
  byte b = 0;
  byte e = RawRead(fd, &b, (word)1, &cc);
  if (e) return e;
  if (cc!=1) return 255;
  *out_b = b;
  return 0;
}

byte RawWrite1(byte fd, byte b) {
  word cc;
  byte e = RawWrite(fd, &b, (word)1, &cc);
  if (e) return e;
  if (cc!=1) return 255;
  return 0;
}
