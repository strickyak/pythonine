#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "octet.h"

#define V_OCTET \
  if (false) fprintf

// Emulated memory for testing on a large host.
// On a 64K platform, prefer raw memory somewhere.
byte ORam[1 << 16];
word ORamUsed;
word ORamFrozen;
word ORamBegin;
word ORamEnd;
omarker OMarkerFn;

// Free chunks are link-listed from OBucket, by word addr.
// Word addr 0 means empty bucket or end of list.
word OBucket[O_NUM_BUCKETS];
byte OBucketCap[] = {2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 254};

bool ovalidaddr(word a) {
  bool z = ((a & 1) == 0 && ORamBegin < a && a < ORamEnd);
#if 1
  if (z) {
    ocheckguards(a);
  }
#endif
  return z;
}

byte ocap(word a) {
  assert(ovalidaddr(a));
  ocheckguards(a);
  byte cap = (0x7F & ogetb(a - DCAP)) << 1;
  assert(cap);
  return cap;
}
byte ocls(word a) {
  assert(ovalidaddr(a));
  ocheckguards(a);
  return ogetb(a - DCLS);
}

byte osize2bucket(byte size) {
  for (byte buck = 0; buck < O_NUM_BUCKETS; buck++) {
    if (size <= OBucketCap[buck]) {
      return buck;
    }
  }
  opanic(OE_TOO_BIG);  // Request was too big.
  return 0;            // Not reached.
}

void oinit(word begin, word end, omarker fn) {
  assert2((int)sizeof(OBucketCap) / (int)sizeof(byte) == O_NUM_BUCKETS,
          "%d should == %d", (int)sizeof(OBucketCap) / (int)sizeof(byte),
          O_NUM_BUCKETS);
  ORamBegin = begin;
  ORamEnd = end;
  OMarkerFn = fn;
  ORamUsed = begin;
  ORamFrozen = begin;
  for (byte i = 0; i < O_NUM_BUCKETS; i++) OBucket[i] = 0;
  ozero(ORamBegin, ORamEnd);
#if 0
#if GUARD
  oputb(ORamBegin, GUARD_ONE);
#endif
#endif
}

void ozero(word begin, word len) {
  for (word i = 0; i < len; i++) oputb(begin + i, 0);
}

void oassertzero(word begin, word len) {
  for (word i = 0; i < len; i++) {
    assert2(!ogetb(begin + i), "nonzero=%d at %d", (int)ogetb(begin + i),
            (int)i);
  }
}

word ocarve(byte len, byte cls) {
  if (!cls) opanic(OE_ZERO_CLASS);
  if (len == 0xFF) opanic(OE_TOO_BIG);
  if (len & 1) ++len;  // align up to even len.  // XXX
  byte buck = osize2bucket(len);
  byte cap = OBucketCap[buck];
  assert(cap >= len);

  // reserve both initial and spare final header.
  word delta = DHDR + cap;
  word final = ORamUsed + delta;
  word final_plus_guard = final + GUARD;
  // detect out of memory.
  if (final_plus_guard >= ORamEnd) return 0;
  // detect overflow (if ORamEnd is very large).
  if (final_plus_guard < ORamUsed) return 0;

  word p = ORamUsed + DHDR;
#if GUARD
  oputb(p - 4, GUARD_ONE);
  oputb(p - 1, GUARD_TWO);
  oputb(final, GUARD_ONE);
#endif
  // TODO: use smaller amount if Forever.
  oputb(p - DCAP, cap >> 1);
  oputb(p - DCLS, cls);
  oassertzero(p, cap);

  ORamUsed = final;
  ocheckguards(p);
  return p;
}

word oallocforever(byte len, byte cls) {
  if (ORamFrozen != ORamUsed) opanic(OE_TOO_LATE);
  word p = ocarve(len, cls);
  if (!p) opanic(OE_OUT_OF_MEM);
  ORamFrozen = ORamUsed;
  ocheckguards(p);
  return p;
}
void ocheckguards(word p) {
#if GUARD
  assert(ogetb(p - 4) == GUARD_ONE);
  assert(ogetb(p - 1) == GUARD_TWO);
  byte cap = ogetb(p - DCAP) << 1;  // don't call ocap()
  assert(cap);
  word final = p + cap;
  assert(ogetb(final) == GUARD_ONE);
  byte cls = ogetb(p - DCLS);  // don't call ocls()
  assert(0 < cls);
  assert(cls < 30);  // XXX
#endif
}
word oalloc_try(byte len, byte cls) {
  if (!cls) opanic(OE_ZERO_CLASS);
  byte buck = osize2bucket(len);
  // Get p from the head of the linked list.
  word p = OBucket[buck];
  if (p) {
#if GUARD
    ocheckguards(p);
#endif
    // Remove p from the head of the linked list.
    OBucket[buck] = ogetw(p);  // next link is at p.
    oputw(p, 0);               // Clear where the link was.
    oputb(p - DCLS, cls);
    word cap = ocap(p);
    assert(cap >= len);
    oassertzero(p, cap);
    V_OCTET(stderr, "reuse: buck=%d cap=%d p=%d cls=%d\n", buck, cap, p, cls);
    return p;
  }

  // Carve a new one.
  return ocarve(len, cls);
}

word oalloc(byte len, byte cls) {
  // First try.
  word p = oalloc_try(len, cls);
  if (!p) {
    // Garbage collect, and try again.
    ogc();
    // Second try.
    p = oalloc_try(len, cls);
  }
  V_OCTET(stderr, "oalloc %d %d -> %d\n", len, cls, p);
  if (!p) opanic(OE_OUT_OF_MEM);
  ocheckguards(p);
  return p;
}

void ofree(word addr) {
#if GUARD
  ocheckguards(addr);
#endif
  byte cap = ocap(addr);
  byte buck = osize2bucket(cap);
  oputb(addr - DCLS, 0);  // Clear the class.
  ozero(addr, cap);       // Clear the data, cap bytes starting at addr.

  // Link the freed cell into the bucket chain.
  oputw(addr, OBucket[buck]);  // In first 2 bytes of cell.
  OBucket[buck] = addr;
}

void omark(word addr) {
  if (addr || addr & 1 || addr < ORamBegin || addr >= ORamEnd) {
    return;
  }

  byte cls = ogetb(addr - 1);
  if (!cls) opanic(OE_ZERO_CLASS);

#if GUARD
  ocheckguards(addr);
#endif

  word cap_ptr = addr - DCAP;
  oputb(cap_ptr, 0x80 | ogetb(cap_ptr));

  if (cls > O_LAST_NONPTR_CLASS) {
    // The payload is pointers.
    word len = ocap(addr);
    for (word i2 = 0; i2 < len; i2 += 2) {
      word q = ogetw(addr + i2);
      if (ovalidaddr(q)) {
        // Looks like q is an object pointer.
        // See if it is marked yet.
        byte qmark = 0x80 & ogetb(q - 2);
        // If not, recurse to mark and visit the object.
        if (!qmark) omark(q);
      }
    }
  }
}

void ogc() {
  V_OCTET(stderr, "ogc: begin {{{\n");
  // Mark all our roots.
  OMarkerFn();

  // TODO: mark from permanent objs.
#if 0
  {word p = ORamBegin;
    while (p < oRamForever) {
	    ......
    }
  }
#endif

  // Reset all the buckets.
  for (byte i = 0; i < O_NUM_BUCKETS; i++) OBucket[i] = 0;

  V_OCTET(stderr, "ogc: sweep ===\n");
  word p = ORamFrozen + DHDR;
  while (p < ORamUsed) {
#if GUARD
    ocheckguards(p);
#endif
    byte cls = ocls(p);
    word cap = ocap(p);
    byte mark_bit = 0x80 & ogetb(p - DCAP);
    // If it's unused (its class is 0 or mark bit is not set):
    // TODO: !cls doesn't mean anything.
    if (!cls || !mark_bit) {
      oputb(p - DCLS, 0);  // Clear class.
      ozero(p, cap);       // Clear payload.
      byte buck = osize2bucket(cap);
      // Add p to front of linked list.
      oputw(p, OBucket[buck]);
      OBucket[buck] = p;
    } else {
      oputb(p - DCAP, ogetb(p - DCAP) & 0x7F);  // clear the mark bit.
    }
    p += cap + DHDR;
  }
  V_OCTET(stderr, "ogc: end }}}\n");
}

char* oshow(word addr) {
  if (!addr) {
    return strdup("nil");
  }
  word cap = ocap(addr);
  char* s = malloc(cap * 3 + 32);
  sprintf(s, "cap=%d[%02x %02x]:", cap, ogetb(addr - DCAP), ogetb(addr - DCLS));
  int n = strlen(s);
  for (int i = 0; i < cap; i += 2) {
    sprintf(s + n + 2 * i + i / 2, " %02x%02x", ogetb(addr + i),
            ogetb(addr + i + 1));
  }
  ocheckguards(addr);
  return s;
}

void osay(word addr) {
  char* s = oshow(addr);
  V_OCTET(stderr, "  @%04x: %s\n", addr, s);
  free(s);
#if GUARD
  ocheckguards(addr);
#endif
}
void osaylabel(word addr, const char* label, int index) {
  char* s = oshow(addr);
  V_OCTET(stderr, "[%s #%d] %04x %s\n", label, index, addr, s);
  free(s);
#if GUARD
  ocheckguards(addr);
#endif
}

void odump() {
  int count_used = 0;
  int bytes_used = 0;
  int count_skip = 0;
  int bytes_skip = 0;
  V_OCTET(stderr, "\n{{{{{ ODUMP %04x:\n", ORamBegin);
  word p = ORamBegin + DHDR;
  while (p < ORamUsed) {
    byte cap = ocap(p);
    byte cls = ocls(p);
    if (cls) {
      ++count_used;
      bytes_used += cap;
      osaylabel(p, "odump", count_used);
    } else {
      ++count_skip;
      bytes_skip += cap;
      V_OCTET(stderr, "skip: %04x [%d.]\n", p, cap);
    }
    p += cap + DHDR;
  }
  V_OCTET(stderr, "count: used=%d + skip=%d = total=%d.\n", count_used,
          count_skip, count_used + count_skip);
  V_OCTET(stderr, "bytes: used=%d + skip=%d = total=%d.\n", bytes_used,
          bytes_skip, bytes_used + bytes_skip);
  V_OCTET(stderr, "ram: used=%d / total=%d = %.2f%%.\n", bytes_used,
          (ORamEnd - ORamBegin), 100.0 * bytes_used / (ORamEnd - ORamBegin));
  V_OCTET(stderr, "ram: carved=%d / total=%d = %.2f%%.\n",
          bytes_used + bytes_skip, (ORamEnd - ORamBegin),
          100.0 * (bytes_used + bytes_skip) / (ORamEnd - ORamBegin));

  V_OCTET(stderr, "}}}}} ODUMP %04x\n\n", ORamUsed);
}

void omemcpy(word d, word s, byte n) {
  for (byte i = 0; i < n; i++) {
    oputb(d, ogetb(s));
    d++;
    s++;
  }
}

int omemcmp(word pchar1, byte len1, word pchar2, byte len2) {
  byte i;
  byte len = (len1 < len2) ? len1 : len2;
  for (i = 0; i < len; i++) {
    if (ogetb(pchar1) < ogetb(pchar2)) return -1;
    if (ogetb(pchar1) > ogetb(pchar2)) return +1;
    ++pchar1, ++pchar2;
  }
  if (len1 > len) return +1;
  if (len2 > len) return -1;
  return 0;
}
