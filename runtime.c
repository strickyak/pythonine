#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "_generated_proto.h"
#include "chain.h"
#include "octet.h"
#include "pb.h"
#include "runtime.h"

// Define strings.  They are in Core Part 2.
#define _CORE_PART_ 2
#include "_generated_core.h"
#undef _CORE_PART_

word Builtins;
word GlobalDict;  // todo: Modules.
word InternList;
word ClassList;
int sp;
word Stack[STACK_SIZE];

byte GetB(word a) { return ogetb(a); }

void PutB(word a, byte x) { oputb(a, x); }

word GetW(word a) { return ogetw(a); }
void PutW(word a, word x) { oputw(a, x); }

void StartQ(word a, byte x) {
  oputb(a, 0xFF);
  oputb(a + 1, x);
}
byte GetQ(word a) {
  assert(ogetb(a) == 0xFF);
  return ogetb(a + 1);
}
void PutQ(word a, byte x) {
  assert(ogetb(a) == 0xFF);
  oputb(a + 1, x);
}

byte Hex(byte b) {
  b = b & 15;
  return (b < 10) ? '0' + b : 'a' + b - 10;
}
void SayObj(word p) {
  printf("<$%x=%d.> ", (int)p, (int)p);
  if (!p) {
    printf("nil");
  } else if (ovalidaddr(p)) {
    const char* clsname = "?";
    byte cls = ocls(p);
    if (cls < (sizeof ClassNames / sizeof *ClassNames)) {
      clsname = ClassNames[cls];
    }
    char* s = oshow(p);
    printf("%s: %s", clsname, s);
    free(s);
  } else if ((p & 0xFF00) == 0xFF00) {
    printf("Q $%02x=%d.", N(p), N(p));
  } else {
    printf("?????");
  }
}

void SayStack() {
  for (int i = 1; i <= sp; i++) {
    word p = Stack[i];
    printf(":::Stack[%d]: ", i);
    SayObj(p);
    printf("\n");
  }
}

void opanic(byte a) {
  fprintf(stderr, "\nopanic: %d\n", a);
  assert(0);
}

word NewBuf() {
  word buf = Buf_NEW();
  word guts = oalloc(254, C_Bytes);  // XXX this is huge
  Buf_bytes_Put(buf, guts);
  return buf;
}
word NewStr(word obj, byte offset, byte len) {
  word str = Str_NEW();
  Str_bytes_Put(str, obj);
  Str_len_Put(str, len);
  Str_offset_Put(str, offset);
  return str;
}
word NewStrCopyFrom(word s, word len) {
  assert(len <= 254);
  // TODO: Bytes pool.
  word obj = oalloc(len, C_Bytes);
  for (word i = 0; i < len; i++) {
    PutB(obj + i, GetB(s + i));
  }
  return NewStr(obj, 0, len);
}

/////////////
char* ShowStr(word a) {
  if (ocls(a) == C_Str) {
    word guts_a = GetW(a);
    word addr_a = guts_a + GetW(a + 2);

    byte len_a = GetW(a + 4);
    char* s = malloc(4 * len_a + 32);
    char* t = s;
    *t++ = '"';
    for (byte i = 0; i < len_a; i++) {
      byte b = GetB(addr_a + i);
      if (32 <= b && b <= 126) {
        *t++ = b;
      } else {
        *t++ = '\\';
        *t++ = Hex(b >> 4);
        *t++ = Hex(b);
      }
    }
    *t++ = '"';
    *t++ = 0;
    return s;
  } else {
    assert1(0, "ShowStr but got cls %d", (int)ocls(a));
  }
}
bool Truth(word a) {
  if (ovalidaddr(a)) {
    switch (ocls(a)) {
      case C_Chain:
      case C_List:
      case C_Dict:
        return Chain_len2(a) > 0;
      case C_Str:
        return Str_len(a) > 0;
    }
  } else {
    if (a == 0) return false;       // None
    if (a == 0xFF00) return false;  // Zero small.
    if (a == 0x0001) return false;  // Zero tagged int.
  }
  return true;
}
bool Eq(word a, word b) {
  printf("Eq(%04x,%04x)[%x,%x]...\n", (int)a, (int)b, (int)ocls(a),
         (int)ocls(b));
  if (a == b) {
    return true;
  }
  if (ovalidaddr(a) && ovalidaddr(b)) {
    // Strings.
    if (ocls(a) == C_Str && ocls(b) == C_Str) {
      {
        char *s1 = ShowStr(a), *s2 = ShowStr(b);
        printf("Eq( %s %s )...\n", s1, s2);
        free(s1), free(s2);
      }

      byte len_a = GetW(a + 4);
      byte len_b = GetW(b + 4);
      if (len_a != len_b) {
        return false;
      }
      word guts_a = GetW(a);
      word guts_b = GetW(b);
      word addr_a = guts_a + GetW(a + 2);
      word addr_b = guts_b + GetW(b + 2);
      for (byte i = 0; i < len_a; i++) {
        if (GetB(addr_a + i) != GetB(addr_b + i)) {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  return false;
}

bool IsQ(word addr) { return (GetW(addr) & 0xFF00) == 0xFF00; }

byte VecSize2(word coll) {
  byte t = ocls(coll);
  assert(t == C_Dict || t == C_List);
  assert(IsQ(coll + 2));
  return GetQ(coll + 2);
}
void VecResize2(word coll, word sz2) {
  assert(sz2 <= 128);
  word guts = GetW(coll);
  word cap = ocap(guts);
  if (cap >= sz2 + sz2) {
    PutQ(coll + 2, sz2);
    return;
  }
  byte oldlen2 = VecSize2(coll);
  word newguts = oalloc(sz2 + sz2, C_Array);
  for (int i = 0; i < oldlen2; i++) {
    word tmp = GetW(guts + i + i);
    PutW(newguts + i + i, tmp);
  }
  PutW(coll, newguts);
  PutQ(coll + 2, sz2);
}

word ListGetAt(word obj, byte at2) {
  word len2 = VecSize2(obj);
  assert2(at2 < len2, "at2=%d [should be <] len2=%d", at2, len2);

  word guts = GetW(obj);
  return GetW(guts + at2 + at2);
}
void ListPutAt(word obj, byte at2, word value) {
  word len2 = VecSize2(obj);
  assert(at2 < len2);

  word guts = GetW(obj);
  return PutW(guts + at2 + at2, value);
}

word DictFindAt(word coll, word key, bool creat) {
  word guts = GetW(coll);
  byte len = GetQ(coll + 2);
  for (word i = 0; i + i < len; i += 4) {
    if (Eq(GetW(guts + i + i), key)) {
      return guts + i + i + 2;
    }
  }
  if (creat) {
    VecResize2(coll, len + 2);
    guts = GetW(coll);
    PutW(guts + len + len, key);
    PutW(guts + len + len + 2, 0);
    return guts + len + len + 2;
  }
  return 0;
}
word FindAt(word coll, word key, bool creat) {
  printf("FindAt(%d):  coll=", (int)creat);
  SayObj(coll);
  printf("  key=");
  SayObj(key);
  printf("\n");
  if (ocls(coll) == C_Dict) {
    return DictFindAt(coll, key, creat);
  } else {
    osaylabel(coll, "FindAt coll", -1);
    assert(0);
  }
}
////////////////////////////////////////

void EvalCodes(word code) {
  SayObj(code);
  byte* codes = &ogetb(code);

  bool done = false;
  for (byte ip = 0; !done; ip++) {
    printf(":::::\n");
    assert(sp >= 0);
    SayStack();
    byte c = codes[ip];
    printf("::::: ip=%d code=%d ((( %s ))) arg=%d sp=%d\n", ip, c, CodeNames[c],
           codes[ip + 1], sp);

    switch (c) {
// Define bytecode cases.  They are in Core Part 3.
#define _CORE_PART_ 3
#include "_generated_core.h"
#undef _CORE_PART_

    }  // end switch

  }  // next ip
}

bool StrEq(word a, word b) {
  assert(ocls(a) == C_Str);
  assert(ocls(b) == C_Str);
  if (Str_len(a) != Str_len(b)) return false;
  assert(Str_bytes(a));
  assert(Str_bytes(b));
  return !omemcmp(Str_bytes(a), Str_len(a), Str_bytes(b), Str_len(b));
}

byte InternString(word str) {
  assert(ocls(str) == C_Str);
  struct ChainIterator it;
  ChainIterStart(InternList, &it);
  // ChainIterNext(InternList, &it);  // skip unused nth==0.
  // byte i = 1;                      // skipped 1 already.
  byte i = 0;
  while (ChainIterMore(InternList, &it)) {
    word s = ChainIterNext(InternList, &it);
    if (StrEq(s, str)) return i;
    ++i;
  }
  ChainAppend(InternList, str);
  return i;
}

word SlurpIntern(word p, word bc, word ilist) {
  byte i_num;
  for (byte tag = GetB(p); tag; tag = GetB(p)) {
    switch (tag) {
      case InternPack_s: {
        word bc, bc_len;
        p = pb_str(p, &bc, &bc_len);
        word str = NewStrCopyFrom(bc, bc_len);
        i_num = InternString(str);
        assert(i_num != INF);
        ChainAppend(ilist, Q(i_num));
      } break;
      case InternPack_patch: {
        word offset;
        p = pb_int(p, &offset);
        assert(offset < ocap(bc));
        PutB(bc + offset, i_num);
      } break;
      default:
        opanic(99);
    }
  }
  return p + 1;  // consume the 0 tag.
}

word SlurpGlobal(word p, word bc, word ilist) {
  byte g_num;
  for (byte tag = GetB(p); tag; tag = GetB(p)) {
    switch (tag) {
      case GlobalPack_name_i: {
        word ith;
        p = pb_int(p, &ith);
        byte i_num = N(ChainGetNth(ilist, ith));
        assert(i_num != INF);
        // TODO -- append to GlobalList.
        // Get the i_numth interned string.
        word s = ChainGetNth(InternList, i_num);
        assert(s);
        // Create a global slot for that string.
        ChainDictPut(GlobalDict, s, None);
        g_num = 1 + ChainDictWhatNth(GlobalDict, s);
        assert(g_num != INF);
      } break;
      case GlobalPack_patch: {
        word offset;
        p = pb_int(p, &offset);
        assert(offset < ocap(bc));
        PutB(bc + offset, g_num);
      } break;
      default:
        opanic(99);
    }
  }
  return p + 1;  // consume the 0 tag.
}

word SlurpModule(word p, word* bc_out) {
  word bc, bc_len;
  word ilist = NewList();
  for (byte tag = GetB(p); tag; tag = GetB(p)) {
    switch (tag) {
      case CodePack_bytecode: {
        word ptr;
        p = pb_str(p, &ptr, &bc_len);
        bc = oalloc(bc_len, C_BC);
        omemcpy(bc, ptr, bc_len);
      } break;
      case CodePack_interns: {
        p = SlurpIntern(p + 1, bc, ilist);
      } break;
      case CodePack_globals: {
        p = SlurpGlobal(p + 1, bc, ilist);
      } break;

      default:
        opanic(99);
    }
  }
  ofree(ilist);
  *bc_out = bc;
  return GlobalDict;  // XXX
}

void RuntimeInit() {
  // Fill in bogus numbers (odd primes, why not) into the
  // first unused slot of these lists.
  Builtins = NewList();
  // ChainAppend(Builtins, Q(11));

  GlobalDict = NewDict();  // todo: Modules.
  // ChainAppend(GlobalDict, Q(13));
  // ChainAppend(GlobalDict, Q(17));

  InternList = NewList();
  // ChainAppend(InternList, Q(19));

  ClassList = NewList();
  // ChainAppend(ClassList, Q(23));
}
