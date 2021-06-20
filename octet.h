#ifndef OCTET_H_
#define OCTET_H_

#ifndef GUARD
#define GUARD 1 /* must be 0 or 1 */
#endif

#define GUARD_ONE 0xAA
#define GUARD_TWO 0xBB

// Header if GUARD:
//   { GUARD_ONE, cap, cls, GUARD_TWO}
// Header if no guard:
//   { cap, cls }
//
// cap field is actually (capacity>>1),
// and the high bit is reserved for mark/sweep gc.
// So the capacity is even, from 0 to 254 inclusive.
//
// cls can be 1 to 255, inclusive.
// cls zero is reserved to mark a free object.
// In the future, cls can be used to determine
// what object pointers the object contains.
// currently, O_LAST_NONPTR_CLASS determines
// whether an object contains all pointers
// or all non-pointer bytes.
// Any cls <= O_LAST_NONPTR_CLASS is non-pointer bytes.
//
// Free lists are used, in around a dozen size
// categories, to speed up allocation.
// Free objects use their first two bytes to form
// the linked list of free objects in their size
// bucket.  The smallest size bucket is 2, so there
// will always be 2 bytes for the link.

// How large is the header.
#define DHDR (2 + 2 * GUARD)
// How many bytes before the addr to find the cap.
#define DCAP (2 + GUARD)
// How many bytes before the addr to find the cls.
#define DCLS (1 + GUARD)

// Octet is composed of 8-bit unsigned bytes.
typedef unsigned char byte;
// Octet is addressed by 16-bit unsigned words.
typedef unsigned short word;
// Type of function to mark roots.
typedef void (*omarker)();

typedef byte bool;
#define true 1
#define false 0

// Global variables hold the global state.
extern byte ORam[1 << 16];
extern word ORamUsed;
extern word ORamFrozen;
extern word ORamBegin;
extern word ORamEnd;
extern omarker OMarkerFn;

#define O_LAST_NONPTR_CLASS 3 /* classes 1, 2, and 3 are bytes */

// public api:

#define ogetb(A) (ORam[(word)(A)])
#define oputb(A, X) (ORam[(word)(A)] = (byte)(X), 0)
#define ogetw(A) ((word)((((word)ogetb(A)) << 8) | (word)ogetb(A + 1)))
#define oputw(A, X) (oputb(A, (word)X >> 8), oputb(A + 1, X), 0)
#define olea(A) (ORam + A)

// Garbage collected alloc & gc.
extern void omark(word addr);  // Mark a root object.

extern void oinit(word begin, word end, omarker fn);
extern word oalloc(byte len, byte cls);
extern void ogc();
extern void ofree(word addr);  // Unsafe.
bool ovalidaddr(word addr);
byte ocap(word addr);  // capacity in bytes.
byte ocls(word addr);
char* oshow(word addr);
void osay(word addr);
void osaylabel(word addr, const char* label, int arg);

extern byte osize2bucket(byte size);
extern void opanic(byte);
extern void ozero(word begin, word len);
extern void oassertzero(word begin, word len);
void omemcpy(word d, word s, byte n);
int omemcmp(word pchar1, byte len1, word pchar2, byte len2);
void odump();
void ocheckguards(word addr);

extern byte ORam[1 << 16];
#define O_NUM_BUCKETS 13

// Error Numbers
#define OE_NULL_PTR 50   /* using a null pointer */
#define OE_BAD_PTR 51    /* using a bad pointer (too large, or not even) */
#define OE_TOO_BIG 52    /* length of alloc cannot be over 256 */
#define OE_TOO_LATE 53   /* cannot call oallocforever() after oalloc() */
#define OE_ZERO_CLASS 54 /* class number can not be zero */
#define OE_OUT_OF_MEM 55 /* out of memory */

#define assert0(C, F)                       \
  {                                         \
    bool _c_ = (C);                         \
    if (!_c_) {                             \
      fflush(stdout);                       \
      fprintf(stderr, "\n***** Failure: "); \
      fprintf(stderr, (F));                 \
      fprintf(stderr, "\n");                \
      assert(0);                            \
    }                                       \
  }

#define assert1(C, F, X)                    \
  {                                         \
    bool _c_ = (C);                         \
    if (!_c_) {                             \
      fflush(stdout);                       \
      fprintf(stderr, "\n***** Failure: "); \
      fprintf(stderr, (F), (X));            \
      fprintf(stderr, "\n");                \
      assert(0);                            \
    }                                       \
  }

#define assert2(C, F, X, Y)                 \
  {                                         \
    bool _c_ = (C);                         \
    if (!_c_) {                             \
      fflush(stdout);                       \
      fprintf(stderr, "\n***** Failure: "); \
      fprintf(stderr, (F), (X), (Y));       \
      fprintf(stderr, "\n");                \
      assert(0);                            \
    }                                       \
  }

#endif  // OCTET_H_
