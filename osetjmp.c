#include "osetjmp.h"

#if !unix

asm word osetjmp(struct ojmp_buf buf) {
  asm {
jb_y equ 0
jb_u equ 2
jb_sp equ 4
jb_pc equ 6

  ldx 2,s ;;; buf

  sty jb_y,x
  stu jb_u,x
  sts jb_sp,x

  ldd 0,s  ; pc
  std jb_pc,x

  clra
  clrb
  rts
  }
}

asm word olongjmp(struct ojmp_buf buf, word retval) {
  asm {
    ldx 2,s ;;; buf
    ldd 4,s ;;; retval

    lds jb_sp,x  ; the old sp, future RTS.

    ldy jb_pc,x
    sty ,s  ; future pc

    ldu jb_u,x  ; future U

    ldy jb_y,x  ; future Y

    rts
  }
}

#endif
