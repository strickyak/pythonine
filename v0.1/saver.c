// O_LAST_NONPTR_CLASS == 5
//
// Format:
//   while
//     addr [2] != 0; do
//     cap [1]
//     cls [1]
//     F0F0 [2]
//     payload [cap]
//     FEFE [2]
//   repeat

// No error returns.  Assume panic on errors.

#include "saver.h"

#include "os9.h"
#include "octet.h"
#include "standard.h"
#include "rawio.h"
#include "_generated_prim.h"

#if unix
void SaveClusterToFile(word top, word filename_str) {
  opanic(37);
}
#else

#define pb(X) checkerr(RawWrite1(fd, X))

void SaveRecursive(word top, byte fd) {
  assert(top);       // cannot be nullptr.
  assert(!(top&1));  // cannot be tagged as small int.
  assert(fd);

  byte cap = ocap(top);
  byte cls = ocls(top);
  assert(cap);       // cap > 0.
  assert(cap < INF); // cap < INF.
  assert(cls);       // cannot be free.

  osaylabel(top, "SaveRecur", 0);

  pb((byte)(top>>8));
  pb((byte)top);
  pb(cap);
  pb(cls);
  pb(0xF0);
  pb(0xF0);

  byte* ptr = (byte*)top;
  byte* end = ptr + cap;
  while (ptr < end) {
    pb(*ptr++);
  }
  pb(0xFE);
  pb(0xFE);

  if (cls <= O_LAST_NONPTR_CLASS) return;
  
  ptr = (byte*)top;  // rewind.
  word w;
  while (ptr < end) {
    w = *(word*)ptr;
    if (w && !(w&1)) {
      SaveRecursive(w, fd);
    }
    ptr += 2;  // step by words, not bytes.
  }
}

void SaveClusterToFile(word top, word filename_str) {
  byte name_len = *(byte*)filename_str;
  byte* name = 1 + (byte*)filename_str;
  assert(name_len > 0);

  // Prepare name with high bit on final char
  name[name_len-1] |= 0x80;

  ocheckall(); printf("@");
  byte fd = 0;
  checkerr(RawCreate((const char*)name, &fd));

  // Unprepare name with high bit on final char
  name[name_len-1] &= 0x7f;

  ocheckall(); printf("@");
  SaveRecursive(top, fd);
  pb(0);  // hi nullptr to terminate
  pb(0);  // lo nullptr
  ocheckall(); printf("@");
  checkerr(RawClose(fd));
  ocheckall(); printf("@");
}

#endif
