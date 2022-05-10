#include "octet.h"

#define V_OCTET \
  if (false) printf

#define V_DUMP \
  if (true) printf

// Emulated memory for testing on a large host.
// On a 64K platform, prefer raw memory somewhere.
#if unix
byte ORam[1 << 16];
#endif
word ORamUsed;
word ORamBegin;
word ORamEnd;
omarker OMarkerFn;
word OGrace1;
word OGrace2;

// Free chunks are link-listed from OBucket, by word addr.
// Word addr 0 means empty bucket or end of list.
word OBucket[O_NUM_BUCKETS];
byte OBucketCap[] = {2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 254};

#if unix
word ogetw(word addr) {
  ocheckall();
  return OGETW(addr);
}
void oputw(word addr, word value) {
  assert(! (value != 0 && ((value&1) != 1) && (value < ORamBegin || value > ORamEnd)) );
  OPUTW(addr, value);
  ocheckall();
}
#endif
word qgetb(word addr) { return ogetb(addr); }
word qgetw(word addr) {
#if unix
  return OGETW(addr);
#else
  return ogetw(addr);
#endif
}

bool ovalidaddr(word p) {
  bool z = ((p & 1u) == 0u && ORamBegin < p && p < ORamEnd);
#if CAREFUL
#if GUARD
  if (z) {
    assert(ogetb(p - 4) == GUARD_ONE);
    assert(ogetb(p - 1) == GUARD_TWO);
  }
#endif
#endif
  return z;
}

void assert_ovalidaddr(word a) {
  if (ovalidaddr(a)) return;
  printf("*** INVALID HANDLE: %x\n", a);
  assert(0);
}

byte ocap(word a) {
#if CAREFUL
  assert_ovalidaddr(a);
  ocheckguards(a);
#endif
  byte cap = (0x7F & ogetb(a - DCAP)) << 1;
  assert(cap);
  return cap;
}
byte ocls(word a) {
  // Fake class 0 for fake pointer 0.
  if (!a) return 0;
#if CAREFUL
  assert_ovalidaddr(a);
  assert_ovalidaddr(a);
  ocheckguards(a);
#endif
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

void odumpsummary() {
  word count_used = 0;
  word bytes_used = 0;
  odump(&count_used, &bytes_used, NULL, NULL);
  printf("STATS: count=%d bytes=%d\n", count_used, bytes_used);
}
void oinit(word begin, word end, omarker fn) {
  begin = 0xFFFE & (begin + 2);  // 2-align up.
  end = 0xFFFE & (end - 1);      // 2-align down.
  V_OCTET("oinit: begin=%x end=%x\n", begin, end);
#if DEBUG
  assert2((int)sizeof(OBucketCap) / (int)sizeof(byte) == O_NUM_BUCKETS,
          "%d should == %d", (int)sizeof(OBucketCap) / (int)sizeof(byte),
          O_NUM_BUCKETS);
#endif
  ORamBegin = begin;
  ORamEnd = end;
  OMarkerFn = fn;
  ORamUsed = begin;
  for (byte i = 0; i < O_NUM_BUCKETS; i++) OBucket[i] = 0;
  V_OCTET("oinit: ORamBegin=%x ORamEnd=%x\n", ORamBegin, ORamEnd);
#if 0
  ozero(ORamBegin, ORamEnd - ORamBegin);
#endif
  V_OCTET("oinit: ORamBegin=%x ORamEnd=%x\n", ORamBegin, ORamEnd);
#if GUARD
  oputb(ORamBegin, GUARD_ONE);
#endif

  // Carve some in each bucket, so none will be void after GC.
  for (byte b =0; b < O_NUM_BUCKETS; b++) {
    for (byte i=0; i<10; i++) {
      oalloc(OBucketCap[b], 1);
    }
  }
}

void ozero(word begin, word len) {
  for (word i = 0; i < len; i++) {
    oputb(begin + i, 0);
  }
}

#if DEBUG
void oassertzero(word begin, word len) {
  for (word i = 0; i < len; i++) {
    assert2(!ogetb(begin + i), "nonzero=%d at %d", (int)ogetb(begin + i),
            (int)i);
  }
}
#endif

// Returns 0 if it cannot carve.
word ocarve(byte len, byte cls) {
#if CAREFUL
  if (!cls) opanic(OE_ZERO_CLASS);
  if (len == 0xFF) opanic(OE_TOO_BIG);
#endif
  if (len & 1) ++len;  // align up to even len.  // XXX
  byte buck = osize2bucket(len);
  byte cap = OBucketCap[buck];
#if CAREFUL
  assert(cap >= len);
  V_OCTET("carve: cls=%d. len=%d. buck=%d. cap=%d.\n", cls, len, buck, cap);
  V_OCTET("carve: ORamUsed=%x ORamEnd=%x\n", ORamUsed, ORamEnd);
#endif

  // reserve both initial and spare final header.
  word delta = (word)DHDR + (word)cap;
  word final = ORamUsed + delta;
  word final_plus_guard = final + GUARD;

  V_OCTET("carve: delta=%x final=%x fpg=%x\n", delta, final, final_plus_guard);

  // detect out of memory.
  if (final_plus_guard >= ORamEnd) {
    V_OCTET("carve: oom -> 0\n");
    return 0;
  }
  // detect overflow (if ORamEnd is very large).
  if (final_plus_guard < ORamUsed) {
    V_OCTET("carve: ov -> 0\n");
    return 0;
  }

  word p = ORamUsed + DHDR;
#if GUARD
  oputb(p - 4, GUARD_ONE);
  oputb(p - 1, GUARD_TWO);
  oputb(final, GUARD_ONE);
#endif
  oputb(p - DCAP, cap >> 1);
  oputb(p - DCLS, cls);
#if DEBUG
//#if CAREFUL
// oassertzero(p, cap);
//#endif
#endif

  ORamUsed = final;
#if CAREFUL
  ocheckguards(p);
#endif
  V_OCTET("carve: ov -> %d\n", p);
  return p;
}

void ocheckguards(word p) {
#if CAREFUL
#if GUARD
  if (p < ORamBegin) printf("ocheckg: p=%d\n", p);
  if (p > ORamUsed) printf("ocheckg: p=%d\n", p);
  assert (p > ORamBegin);
  assert (p < ORamUsed);
  if (ogetb(p - 4) != GUARD_ONE) printf("ocheckg: p=%d\n", p);
  if (ogetb(p - 1) != GUARD_TWO) printf("ocheckg: p=%d\n", p);
  assert(ogetb(p - 4) == GUARD_ONE);
  assert(ogetb(p - 1) == GUARD_TWO);
  byte cap = (0x7F & ogetb(p - DCAP)) << 1;  // don't call ocap()
  if (!cap) printf("ocheckg: p=%d\n", p);
  assert(cap);
  word final = p + cap;
  if (ogetb(final) != GUARD_ONE) printf("ocheckg: p=%d\n", p);
  assert(ogetb(final) == GUARD_ONE);
#endif
#endif
}
// returns 0 if it fails.
word oalloc_try(byte len, byte cls, bool try_bigger) {
  if (!cls) opanic(OE_ZERO_CLASS);
  byte best_bucket = osize2bucket(len);
  for (byte buck = best_bucket; buck < O_NUM_BUCKETS; buck++) {
      V_OCTET("try: cls=%d. len=%d. buck=%d.\n", cls, len, buck);
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
#if CAREFUL
        assert(cap >= len);
#endif
        if (buck != best_bucket) V_OCTET("UPGRADE len %d to cap %d\n", len, cap);
        V_OCTET("reuse: buck=%d cap=%d p=%d cls=%d\n", buck, cap, p, cls);
        return p;
      } else if (!try_bigger) {
        break;
      }
  }

  // Carve a new one.
  return ocarve(len, cls);
}

word oalloc(byte len, byte cls) {
  // First try.
  word p = oalloc_try(len, cls, false);
  if (!p) {
    // Garbage collect, and try again.
    ogc();
    // Second try.
    p = oalloc_try(len, cls, true);
  }
  V_OCTET("oalloc %d %d -> %d\n", len, cls, p);
  if (!p) {
    printf("oom: %d %d\n", len, cls);
    odumpsummary();
    opanic(OE_OUT_OF_MEM);
  }
#if CAREFUL
  ocheckguards(p);
#endif

#if 1
  ozero(p, ocap(p));  // Clear payload.
#else
  memset(p, 0, ocap(p));
#endif

  // Grace period against GC for 2 recent objs.
  OGrace2 = OGrace1;
  OGrace1 = p;
#if CAREFUL
  ocheckguards(p);  // extra careful
#endif
  return p;
}

void ofree(word addr) {
#if GUARD
  ocheckguards(addr);
#endif
  byte cap = ocap(addr);
  byte buck = osize2bucket(cap);
  oputb(addr - DCLS, 0);  // Clear the class.
#if 0
  ozero(addr, cap);       // Clear the data, cap bytes starting at addr.
#endif

  // Link the freed cell into the bucket chain.
  oputw(addr, OBucket[buck]);  // In first 2 bytes of cell.
  OBucket[buck] = addr;
  // Don't leave Grace pointing to freed block.
  if (OGrace1 == addr) OGrace1=0;
  if (OGrace2 == addr) OGrace2=0;
}

struct prev {
  word obj;
  struct prev* next;
};

word omark_recursive(word addr, word depth, struct prev* prev) {
  word max_depth;
TAILCALL:
  max_depth = depth;
  if (!ovalidaddr(addr)) return max_depth;

  byte cls = ogetb(addr - DCLS);
  if (!cls) opanic(OE_ZERO_CLASS);

#if GUARD
  ocheckguards(addr);
#endif
  {
      struct prev item;
      item.obj = addr;
      item.next = prev;
      if (depth > 20) {
        printf("\nPrev: ");
        for (struct prev* p = &item; p; p = p->next) {
          printf("%04x:%d.:%d. ", item.obj, ocap(item.obj), ocls(item.obj));
        }
      }

      word cap_ptr = addr - DCAP;
      byte raw_cap = ogetb(cap_ptr);
      oputb(cap_ptr, 0x80u | raw_cap);

      if (cls > O_LAST_NONPTR_CLASS) {
        // The payload is pointers.
        word len = ocap(addr);
        for (word i2 = 0; i2 < len; i2 += 2) {
          word q = ogetw(addr + i2);
          if (ovalidaddr(q)) {
            // Looks like q is an object pointer.
            // See if it is marked yet.
            byte qmark = 0x80 & ogetb(q - DCAP);
            // If not, recurse to mark and visit the object.
            if (!qmark) {
                if (i2+2 == len) {
                  addr = q;
                  goto TAILCALL;
                }
                word d = omark_recursive(q, depth+1, &item);
                max_depth = (d > max_depth) ? d : max_depth; // max
            }
          }
        }
      }
  }
  return max_depth;
}
void omark(word addr) {
  word max_depth = 0;
  if (ovalidaddr(addr)) {
    word d = omark_recursive(addr, 0, NULL);
    max_depth = (d > max_depth) ? d : max_depth; // max
  }
}

void ogc() {
  printf("{G");
  // Mark all our roots.
  omark(OGrace1);
  omark(OGrace2);
  OMarkerFn();
  printf("C");

  // Reset all the buckets.
  for (byte i = 0; i < O_NUM_BUCKETS; i++) OBucket[i] = 0;

  word p = ORamBegin + DHDR;
  while (p < ORamUsed) {
#if GUARD
    ocheckguards(p);
#endif
    byte cls = ocls(p);
    byte cap = ocap(p);
    byte mark_bit = 0x80 & ogetb(p - DCAP);
    if (!(0x80 & ogetb(p - DCAP))) {
      // avoid writing if already OK, to avoid MOOH VGA damage.
      if (ogetb(p-DCLS)) oputb(p - DCLS, 0);  // Clear class.
      byte buck = osize2bucket(cap);
      // Add p to front of linked list.
      // avoid writing if already OK, to avoid MOOH VGA damage.
      if (ogetw(p) != OBucket[buck]) oputw(p, OBucket[buck]);
      OBucket[buck] = p;
    } else {
      oputb(p - DCAP, ogetb(p - DCAP) & 0x7F);  // clear the mark bit.
    }
    p += (word)cap + (word)DHDR;
  }
  printf("}");
}

void osayhexlabel(word p, word len, char* label) {
  printf("\n(((# ((%s)) addr=%04x len=%04x: ", label, p, len);
  for (word i = 0; i < len; i++) {
    printf("%02x ", ogetb(p + i));
  }
  printf("#)))\n");
}
void osaylabel(word p, const char* label, int index) {
  printf("\n(((* addr=%04x ", p);
  printf("((%s~~%d)) ", label, index);
  osay(p);
  printf("*)))\n");
}
void osay(word p) {
  if (!p) {
    printf("@nil\n");
  }
  if (ovalidaddr(p)) {
    byte cap = ocap(p);
    byte cls = ocls(p);
    printf("@%04x[%d. %d.]\n", p, cap, cls);
    for (int i = 0; i < cap; i += 2) {
      printf(" %02x%02x", ogetb(p + i), ogetb(p + i + 1));
    }
  } else {
    printf("@%04x\n", p);
  }
  printf("\n");
}

void ocheckall() {
  // printf(" (%x .. %x) [[[ ", ORamBegin, ORamUsed);
  // printf("[");
  word p = ORamBegin + DHDR;
  while (p < ORamUsed) {
    // printf("%x ", p);
    // printf("^");
    ocheckguards(p);
    byte cap = (byte)(qgetb(p - DCAP) << 1);
    byte cls = (byte)(qgetb(p - DCLS));
    // printf("(%d# %d) ", cap, cls);
    if (cls) {
      if (cls > O_LAST_NONPTR_CLASS) {
        for (byte i = 0; i < cap; i += 2) {
          word x = qgetw(p + i);
          if (x) {
            if (ovalidaddr(x)) {
              ocheckguards(x);
            } else {
              assert(x & 1);
            }
          }
        }
      }
    }
    // printf(" +%d +%d ", (word)cap, (word)DHDR);
    p += (word)cap + (word)DHDR;
  }
  // printf(" ]]] ");
  // printf("]");
}
void odump(word* count_used_ptr, word* bytes_used_ptr, word* count_skip_ptr,
           word* bytes_skip_ptr) {
  word count_used = 0;
  word bytes_used = 0;
  word count_skip = 0;
  word bytes_skip = 0;
  // V_DUMP("\n{{{{{ ODUMP %04x:\n", ORamBegin);
  word p = ORamBegin + DHDR;
  // V_DUMP("1p=%04x\n", p);
  while (p < ORamUsed) {
    // V_DUMP("2p=%04x...\n", p);
    byte cap = ocap(p);
    // V_DUMP("2p=%04x cap=%02x...\n", p, cap);
    byte cls = ocls(p);
    // V_DUMP("2p=%04x\n cap=%02x cls=%02x\n", p, cap, cls);
    if (cls) {
      ++count_used;
      bytes_used += cap;
#if 0
      osaylabel(p, "odump", count_used);
#endif
    } else {
      ++count_skip;
      bytes_skip += cap;
      // V_DUMP("skip: %04x [%d.]\n", p, cap);
    }
    p += (word)cap + (word)DHDR;
  }
  {
    for (byte i = 0; i < O_NUM_BUCKETS; i++) {
      int count = 0;
      for (word p = OBucket[i]; p; p = ogetw(p)) {
        ++count;
      }
      V_DUMP("B%d[%d]Free:%d ", i, OBucketCap[i], count);
    }
    V_DUMP("\n");
  }
  V_DUMP("count: used=%d + skip=%d = total=%d.\n", count_used, count_skip,
         count_used + count_skip);
  V_DUMP("bytes: used=%d + skip=%d = total=%d.\n", bytes_used, bytes_skip,
         bytes_used + bytes_skip);

#if unix
  V_DUMP("ram: used=%d / total=%d = %.2f%%.\n", bytes_used,
         (ORamEnd - ORamBegin), 100.0 * bytes_used / (ORamEnd - ORamBegin));
  V_DUMP("ram: carved=%d / total=%d = %.2f%%.\n", bytes_used + bytes_skip,
         (ORamEnd - ORamBegin),
         100.0 * (bytes_used + bytes_skip) / (ORamEnd - ORamBegin));
#endif

  // V_DUMP("}}}}} ODUMP %04x\n\n", ORamUsed);
  if (count_used_ptr) {
    *count_used_ptr = count_used;
  }
  if (bytes_used_ptr) {
    *bytes_used_ptr = bytes_used;
  }
  if (count_skip_ptr) {
    *count_skip_ptr = count_skip;
  }
  if (bytes_skip_ptr) {
    *bytes_skip_ptr = bytes_skip;
  }
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

#if 0
void ofatal(const char* f, word x, word y) {
#if unix
  fflush(stdout);
  fprintf(stderr, "\n***** ofailure: ");
  fprintf(stderr, f, x, y);
  fprintf(stderr, "\n");
#else
  printf("\n\n*** ofailure *** ");
  printf(f, x, y);
  printf("\n");
  fatal_coredump();
#endif
  exit(13);
}
#endif

#if !unix
asm fatal_coredump() {
  asm {
    SWI
    FCB $FF
  }
}
#endif
