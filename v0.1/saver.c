// O_LAST_NONPTR_CLASS == 5
//
// Format:
//   0312 [2]
//   while
//     addr [2] != 0; do
//     cap [1]
//     cls [1]
//     F0F0 [2]
//     payload [cap]
//     FEFE [2]
//   repeat
//   0000 [2]

// No error returns.  Assume panic on errors.

#include "saver.h"

#include "os9.h"
#include "os9errno.h"
#include "octet.h"
#include "standard.h"
#include "rawio.h"
#include "_generated_prim.h"

#if unix
void SaveClusterToFile(word top, word filename_str) {
  opanic(37);
}
word LoadClusterFromFile(word filename_str) {
  opanic(38);
}
#else

#define pb(X) checkerr(RawWrite1(fd, X))

#define gb() getByte(fd)

#ifdef ADD_SAVER
byte getByte(byte fd) {
  byte b;
  checkerr(RawRead1(fd, &b));
  // printf(" %x,", b);
  return b;
}
word getWord(byte fd) {
  byte hi = gb();
  byte lo = gb();
  word z = (((word)hi)<<8) + (word)lo;
  // printf(" [%x,%x = %x] ", hi, lo, z );
  return z;
}

word LoadRecursive(byte fd) {
  word id = getWord(fd);
  // printf(" id=$%x ", id);
  assert(id);
  byte cap = gb();
  byte cls = gb();

  word p = oalloc(cap, cls);
  byte* ptr = (byte*)p;
  byte* end = ptr + cap;

  word magic = getWord(fd);
  asserteq(magic, 0xF0F0);
  while (ptr < end) {
    *ptr++ = gb();
  }
  magic = getWord(fd);
  asserteq(magic, 0xFEFE);

  if (cls <= O_LAST_NONPTR_CLASS) return p;

  ptr = (byte*)p;  // rewind.
  word w, w2;
  while (ptr < end) {
    w = *(word*)ptr;
    if (w && !(w&1)) {
      // Overwrite old value with newly loaded object.
      *(word*)ptr = LoadRecursive(fd);
    }
    ptr += 2;  // step by words, not bytes.
  }
  return p;
}

void SaveRecursive(word top, byte fd) {
  assert(top);       // cannot be nullptr.
  assert(!(top&1));  // cannot be tagged as small int.
  assert(fd);

  byte cap = ocap(top);
  byte cls = ocls(top);
  // assert(cap);       // cap > 0.
  // assert(cap < INF); // cap < INF.
  // assert(cls);       // cannot be free.

  // osaylabel(top, "SaveRecur", 0);

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
#endif

void SaveClusterToFile(word top, word filename_str) {
#ifdef ADD_SAVER
  byte name_len = *(byte*)filename_str;
  byte* name = 1 + (byte*)filename_str;
  assert(name_len > 0);

  // Prepare name with high bit on final char
  name[name_len-1] |= 0x80;

  //ocheckall(); printf("@");
  byte fd = 0;
  checkerr(RawCreate((const char*)name, &fd));

  // Unprepare name with high bit on final char
  name[name_len-1] &= 0x7f;

  //ocheckall(); printf("@");
  pb(0x03);  // hi magic number
  pb(0x12);  // lo magic number
  SaveRecursive(top, fd);
  pb(0);  // hi nullptr to terminate
  pb(0);  // lo nullptr
  //ocheckall(); printf("@");
  checkerr(RawClose(fd));
  //ocheckall(); printf("@");
#endif
}

word LoadClusterFromFile(word filename_str) {
  word w = 0;
#ifdef ADD_SAVER
  byte name_len = *(byte*)filename_str;
  byte* name = 1 + (byte*)filename_str;
  assert(name_len > 0);

  // Prepare name with high bit on final char
  name[name_len-1] |= 0x80;

  //ocheckall(); printf("@");
  byte fd = 0;
  checkerr(RawOpen((const char*)name, &fd));

  // Unprepare name with high bit on final char
  name[name_len-1] &= 0x7f;

  assert(gb() == 0x03);  // hi magic number
  assert(gb() == 0x12);  // lo magic number
  w = LoadRecursive(fd);
  assert(gb() == 0);  // hi termination
  assert(gb() == 0);  // lo termination

  checkerr(RawClose(fd));
#endif
  return w;
}

#endif
