#include "runtime.h"

#include "_generated_proto.h"
#include "arith.h"
#include "chain.h"
#include "defs.h"
#include "octet.h"
#include "pb2.h"
#include "readbuf.h"

// GC Roots:
word ForeverRoot;
word RootForMain;  // For main() to use.

word Builtins;
word GlobalDict;  // todo: Modules.
word InternList;
word ClassList;
word MessageList;

byte* codes;     // ????
word codes_obj;  // ????

word function;
word ip;  // Stores in Frame as ip - function.
word fp;
word sp;  // Stores in Frame as sp - fp.

#define VERB \
  if (true) printf

byte Hex(byte b) {
  b = b & 15;
  return (b < 10) ? '0' + b : 'a' + b - 10;
}
void SayChain(word p) {
  struct ChainIterator iter;
  ChainIterStart(p, &iter);
  for (int i = 0; ChainIterMore(p, &iter); i++) {
    word e = ChainIterNext(p, &iter);
    printf(" [[%d]] ", i);
    SayObj(e, 2);
  }
  fflush(stdout);
}
void SayStr(word p) {
  assert(ocls(p) == C_Str);
  printf("'");
  for (byte i = 0; i < Str_len(p); i++) {
    byte ch = Bytes_flex_At(Str_bytes(p), Str_offset(p) + i);
    if (' ' <= ch && ch <= '~') {
      printf("%c", ch);
    } else {
      printf("?");
    }
  }
  printf("'");
  fflush(stdout);
}
void SayObj(word p, byte level) {
  printf("{ <$%x=%d.> ", (int)p, (int)p);
  if (level < 1) return;

  if (!p) {
    printf("nil");
  } else if (p & 1) {
    word x = TO_INT(p);
    printf("int: $%02x=%d.", x, x);
  } else if (ovalidaddr(p)) {
    const char* clsname = "?";
    byte cls = ocls(p);
    if (cls == C_Str) {
      SayStr(p);
    } else {
      if (cls < ClassNames_SIZE) {
        clsname = ClassNames[cls];
        printf("cls %s: cap=%d", clsname, ocap(p));
        if (level < 2) return;
        if (cls == C_Chain || cls == C_List) {
          struct ChainIterator it;
          ChainIterStart(p, &it);
          printf(" list:\n");
          for (byte i = 0; ChainIterMore(p, &it); ++i) {
            word e = ChainIterNext(p, &it);
            printf("\n  [%d]: ", i);
            SayObj(e, level - 1);
          }
          printf("\n");
        } else if (cls == C_Dict) {
          struct ChainIterator it;
          ChainIterStart(p, &it);
          printf(" dict:\n");
          for (byte i = 0; ChainIterMore(p, &it); ++i) {
            word k = ChainIterNext(p, &it);
            word v = ChainIterNext(p, &it);
            printf("\n  [[[");
            SayObj(k, level - 1);
            printf("]]] = ");
            SayObj(v, level - 1);
          }
          printf("\n");
        }
      } else if (cls < Chain_len2(ClassList)) {
        word clsobj = ChainGetNth(ClassList, cls);
        if (clsobj) {
          printf("cls=");
          SayStr(Class_className(clsobj));
          printf(" cap=%d", ocap(p));
        } else {
          printf(" unknown:cls=%d", cls);
        }
      } else {
        osay(p);
      }
    }
  } else {
    printf("??");
  }
  printf("}");
  fflush(stdout);
}

void SayStack() {
  byte cap = ocap(fp);
  int i = 0;
  for (word p = sp; p < fp + cap; p += 2) {
    word x = ogetw(p);
    printf(":::::  [%d]  ", p);
    SayObj(x, 2);
    printf("\n");
    ++i;
  }
  fflush(stdout);
}

#if 0  // unused for now
word NewBuf() {
  word buf = Buf_NEW();
  word guts = oalloc(254, C_Bytes);  // XXX this is huge
  Buf_bytes_Put(buf, guts);
  return buf;
}
#endif
word NewStr(word obj, byte offset, byte len) {
  word z = oalloc(Str_Size, C_Str);
  Str_bytes_Put(z, obj);
  Str_len_Put(z, len);
  Str_offset_Put(z, offset);
  return z;
}
#if 0
word NewStrCopyFrom(word s, byte len) {
  assert(len <= 254);
  word obj = oalloc(len, C_Bytes);
  for (word i = 0; i < len; i++) {
    PutB(obj + i, GetB(s + i));
  }
  return NewStr(obj, 0, len);
}
#endif
word NewStrCopyFromC(const char* s) {
  int len = strlen(s);
  assert(len <= 254);
  word obj = oalloc(len, C_Bytes);
  for (word i = 0; i < len; i++) {
    PutB(obj + i, s[i]);
  }
  return NewStr(obj, 0, len);
}

/////////////
#if 0
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
#endif
bool Truth(word a) {
  if (ovalidaddr(a)) {
    switch (ocls(a)) {
      case C_Chain:
      case C_List:
      case C_Dict:
        return (Chain_len2(a) > 0);
      case C_Str:
        return (Str_len(a) > 0);
    }
  } else {
    if (a == 0) return false;       // None
    if (a == 0x0001) return false;  // Small int 0.
  }
  return true;
}

bool StrEqual(word a, word b) {
  // printf("StrEqual: ");
  // SayStr(a);
  // printf(", ");
  // SayStr(b);
  // printf(": ");

  int len_a = Str_len(a);
  int len_b = Str_len(b);
  if (len_a != len_b) {
    goto FALSE;
  }
  word guts_a = Str_bytes(a);
  word guts_b = Str_bytes(b);
  word addr_a = guts_a + (word)Str_offset(a);
  word addr_b = guts_b + (word)Str_offset(b);

  assert(0 <= len_a && len_a <= 254);
  byte n = (byte)len_a;

  for (byte i = 0; i < n; i++) {
    if (GetB(addr_a + i) != GetB(addr_b + i)) {
      goto FALSE;
    }
  }
  // printf("1\n");
  return true;

FALSE:
  // printf("0\n");
  return false;
}

bool Equal(word a, word b) {
  if (a == b) {
    return true;
  }
  if ((byte)a & 1) return false;
  if ((byte)b & 1) return false;
  if (ovalidaddr(a) && ovalidaddr(b)) {
    if (ocls(a) == C_Str && ocls(b) == C_Str) {
      // Strings.
      return StrEqual(a, b);
    }
    return false;
  }
  return false;
}

////////////////////////////////////////

// class Frame:
//   oop prev_frame
//   small nargs
//   small prev_sp
//   small prev_ip
//   oop[] flex

void EvalCodes(word fn) {
  word fp0 = oalloc(32, C_Frame);
  byte cap0 = ocap(fp0);
  oputw(fp0 + cap0 - 2, fn);
  // Frame_flex_AtPut(fp0, fn, 15);

  fp = oalloc(32, C_Frame);
  byte cap = ocap(fp);
  Frame_prev_frame_Put(fp, fp0);
  Frame_nargs_Put(fp, 0);
  Frame_prev_sp_Put(fp, cap - 2);
  Frame_prev_ip_Put(fp, 0);
  sp = fp + ocap(fp);
  function = fn;
  ip = function + BC_HEADER_SIZE;

  // SayObj(fn, 2);
  RunLoop();
  printf("\n[[[ Finished RunLoop ]]]\n");
}

void RunLoop() {
  while (1) {
    VERB("\n");
    assert(sp >= fp + Frame_Size - 2);
    assert(sp <= fp + ocap(fp));
    SayStack();  // ===========
    assert(ip >= function + BC_HEADER_SIZE);
    assert(ip < function + INF);
    byte opcode = ogetb(ip);
    assert(opcode < CodeNames_SIZE);
    VERB("::::: f=%d ip~%d opcode=%d ((( %s ))) args=%d,%d fp=%d sp~%d\n",
         function, ip - function, opcode, CodeNames[opcode], ogetb(ip + 1),
         ogetb(ip + 2), fp, (sp - fp) >> 1);

  SWITCH:
    assert(fp);
    assert(function);
    assert(sp > fp);
    assert(ip > function);
    assert(sp <= fp + INF);
    assert(ip < function + INF);
    switch (opcode) {
#define _CORE_PART_ 3
#include "_generated_core.h"
#undef _CORE_PART_
      default:
        opanic(240);
    }  // end switch
    ++ip;
  }  // next ip
}

void RunBuiltinMethod(byte meth_num) {
  switch (meth_num) {
#define _CORE_PART_ 4
#include "_generated_core.h"
#undef _CORE_PART_
    default:
      opanic(241);
  }  // end switch
}

byte InternString(word str) {
  assert(ocls(str) == C_Str);
  struct ChainIterator it;
  ChainIterStart(InternList, &it);
  byte i = 0;
  while (ChainIterMore(InternList, &it)) {
    word s = ChainIterNext(InternList, &it);
    if (StrEqual(s, str)) return i;
    ++i;
  }
  ChainAppend(InternList, str);
  return i;
}

void SlurpIntern(struct ReadBuf* bp, word bc, word ilist) {
  byte i_num = INF;
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case InternPack_s: {
        byte bytes_len;
        word bytes = pb_str(bp, C_Bytes, &bytes_len);
        assert(bytes_len < INF);
        word str = NewStr(bytes, 0, bytes_len);
        i_num = InternString(str);
        assert(i_num < INF);
        ChainAppend(ilist, Q(i_num));
      } break;
      case InternPack_patch: {
        word offset = pb_int(bp);
        assert(offset < ocap(bc));
        assert(i_num < INF);
        PutB(bc + offset, i_num);
      } break;
      default:
        opanic(94);
    }
  }
  pb_next(bp);  // consume the 0 tag.
}

void SlurpGlobal(struct ReadBuf* bp, word bc, word ilist) {
  byte g_num;
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case GlobalPack_name_i: {
        word ith = pb_int(bp);
        assert(ith < INF);
        byte i_num = (byte)N(ChainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // TODO -- append to GlobalList.
        // Get the i_numth interned string.
        word s = ChainGetNth(InternList, i_num);
        assert(s);
        // Create a global slot for that string.
        ChainDictPut(GlobalDict, s, None);
        g_num = 1 + ChainDictWhatNth(GlobalDict, s);
        assert(g_num != INF);
        // osaylabel(bc, "GlobalPack_name_i", ith);
      } break;
      case GlobalPack_patch: {
        word offset = pb_int(bp);
        assert(offset < ocap(bc));
        PutB(bc + offset, g_num);
      } break;
      default:
        opanic(98);
    }
  }
  pb_next(bp);  // consume the 0 tag.
}

void SlurpCodePack(struct ReadBuf* bp, word* bc_out) {
  SlurpModule(bp, bc_out);
}

void SlurpFuncPack(struct ReadBuf* bp, word ilist, word dict) {
  word name_str;
  word ith;
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case FuncPack_name_i: {
        ith = pb_int(bp);
        byte i_num = (byte)N(ChainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // Get the i_numth interned string.
        name_str = ChainGetNth(InternList, i_num);
        assert(name_str);
        printf("SLURPING FUNC: <<<");
        SayObj(name_str, 2);
        printf(">>>\n");
      } break;
      case FuncPack_pack: {
        word bc;
        pb_next(bp);
        SlurpCodePack(bp, &bc);
        ChainDictPut(dict, name_str, bc);
        osaylabel(bc, "FuncPack_name_i", ith);
      } break;
      default:
        opanic(97);
    }
  }
  pb_next(bp);  // consume the 0 tag.
}
void SlurpClassPack(struct ReadBuf* bp, word ilist) {
  word class_num = List_len2(ClassList);
  word name_str;
  word ith;
  word field_list = NewList();
  word meth_dict = NewDict();

  word cls = oalloc(Class_Size, C_Class);
  Class_classNum_Put(cls, class_num);
  Class_fieldList_Put(cls, field_list);
  Class_methDict_Put(cls, meth_dict);
  ChainAppend(ClassList, cls);

  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case ClassPack_name_i: {
        ith = pb_int(bp);
        byte i_num = (byte)N(ChainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // Get the i_numth interned string.
        name_str = ChainGetNth(InternList, i_num);
        assert(name_str);
        printf("SLURPING CLASS: <<<");
        SayObj(name_str, 2);
        printf(">>>\n");
        Class_className_Put(cls, name_str);
      } break;
      case ClassPack_field_i: {  // For a field.
        word field_ith = pb_int(bp);
        byte field_i_num = (byte)N(ChainGetNth(ilist, (byte)field_ith));
        assert(field_i_num < INF);
        word field_str = ChainGetNth(InternList, field_i_num);
        assert(field_str);
        printf("SLURPING FIELD <<<");
        SayObj(field_str, 2);
        printf(">>>\n");
        ChainAppend(field_list, field_str);
      } break;
      case ClassPack_meth: {  // For a method.
        pb_next(bp);
        SlurpFuncPack(bp, ilist, meth_dict);
      } break;
      default:
        opanic(96);
    }
  }

  {
    word dunder_init_isn = InternString(NewStrCopyFromC("__init__"));
    assert(dunder_init_isn != INF);
    word dunder_init_name = ChainGetNth(InternList, dunder_init_isn);
    word init_meth = ChainDictGet(meth_dict, dunder_init_name);
    assert(init_meth);
    assert(ocls(init_meth) == C_Bytecodes);

    // Hand craft the constructor.
    // TODO: call __init__.
    word ctor = oalloc(16, C_Bytecodes);
    Bytecodes_flex_AtPut(ctor, 3, INF);
    Bytecodes_flex_AtPut(ctor, 4, INF);
    Bytecodes_flex_AtPut(ctor, 5, INF);
    Bytecodes_flex_AtPut(ctor, 6, BC_Construct);
    Bytecodes_flex_AtPut(ctor, 7, class_num);

    Bytecodes_flex_AtPut(ctor, 8, BC_Dup);  // will return inst
    Bytecodes_flex_AtPut(ctor, 9, BC_CallMeth);
    Bytecodes_flex_AtPut(ctor, 10, dunder_init_isn);
    Bytecodes_flex_AtPut(ctor, 11, 1);        // only arg is `self`
    Bytecodes_flex_AtPut(ctor, 12, BC_Drop);  // result of __init__

    Bytecodes_flex_AtPut(ctor, 13, BC_Return);

    ChainDictPut(GlobalDict, name_str, ctor);
  }

  pb_next(bp);  // consume the 0 tag.
}

void SlurpModule(struct ReadBuf* bp, word* bc_out) {
  word bc;
  word ilist = NewList();
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    // printf("SM: tag=$%x=%d.\n", tag, tag);
    switch (tag) {
      case CodePack_bytecode: {
        bc = pb_str(bp, C_Bytecodes, NULL);
      } break;
      case CodePack_interns: {
        pb_next(bp);
        SlurpIntern(bp, bc, ilist);
      } break;
      case CodePack_globals: {
        pb_next(bp);
        SlurpGlobal(bp, bc, ilist);
      } break;
      case CodePack_funcpacks: {
        pb_next(bp);
        SlurpFuncPack(bp, ilist, GlobalDict);
      } break;
      case CodePack_classpacks: {
        pb_next(bp);
        SlurpClassPack(bp, ilist);
      } break;

      default:
        opanic(95);
    }
  }
  ofree(ilist);
  assert(bc_out);
  *bc_out = bc;
  pb_next(bp);  // consume the 0 tag.
}

void MarkRoots() {
  omark(ForeverRoot);
  omark(RootForMain);
  omark(function);
  omark(fp);
}
void DumpStats() {
  word count_used = 0;
  word bytes_used = 0;
  odump(&count_used, &bytes_used, NULL, NULL);
  printf("STATS: count=%d bytes=%d\n", count_used, bytes_used);
}
void RuntimeInit() {
  // Protect roots forever from GC.
  // Size 10 is twice the number of elements.
  ForeverRoot = oalloc(10, C_Array);

  Builtins = NewList();
  Array_flex_AtPut(ForeverRoot, 0, Builtins);

  GlobalDict = NewDict();  // todo: Modules.
  Array_flex_AtPut(ForeverRoot, 1, GlobalDict);

  InternList = NewList();
  Array_flex_AtPut(ForeverRoot, 2, InternList);

  ClassList = NewList();
  Array_flex_AtPut(ForeverRoot, 3, ClassList);

  MessageList = NewList();
  Array_flex_AtPut(ForeverRoot, 4, MessageList);

  // Reserve builtin class slots, with None.
  ChainAppend(ClassList, None);  // unused 0.
  DumpStats();
  VERB("<%d>", __LINE__);
  for (byte i = 1; i < ClassNames_SIZE; i++) {
    word cls = oalloc(Class_Size, C_Class);
    VERB("<%d>", __LINE__);
    Class_classNum_Put(cls, i);
    VERB("<%d>", __LINE__);
    const char* cstr = 2 /* skip `C_` */ + ClassNames[i];
    word name = NewStrCopyFromC(cstr);
    word name_isn = InternString(name);
    VERB("<%d>", __LINE__);
    name = ChainGetNth(InternList, name_isn);
    VERB("<%d>", __LINE__);
    Class_className_Put(cls, name);
    VERB("<%d>", __LINE__);
    Class_methDict_Put(cls, NewDict());
    VERB("<%d>", __LINE__);
    ChainAppend(ClassList, cls);
  }
  VERB("<%d>", __LINE__);
  for (byte i = 0; i < MessageNames_SIZE; i++) {
    VERB("<%d>", __LINE__);
    word name = NewStrCopyFromC(MessageNames[i]);
    VERB("<%d>", __LINE__);
    // SayStr(name);
    word name_isn = InternString(name);
    VERB("<%d>", __LINE__);
    // SayStr(ChainGetNth(InternList, name_isn));
    VERB("<%d>", __LINE__);
    ChainAppend(MessageList, ChainGetNth(InternList, name_isn));
    VERB("<%d>", __LINE__);
  }
  VERB("<%d>", __LINE__);
  {
    byte i = 0;
    for (const byte* p = BuiltinClassMessageMeths; *p; p += 2, ++i) {
      VERB("<%d>", __LINE__);
      VERB("p=%x p0=%x p1=%x p2=%x p3=%x i=%d.\n", p, p[0], p[1], p[2], p[3],
           i);
      word cls = ChainGetNth(ClassList, p[0]);
      VERB("<%d>", __LINE__);
      word meth_list = Class_methDict(cls);
      VERB("<%d>", __LINE__);
      word message = ChainGetNth(MessageList, p[1]);
      VERB("<%d>", __LINE__);
      ChainAppend(meth_list, message);
      VERB("<%d>", __LINE__);
      ChainAppend(meth_list, FROM_INT(i));
      VERB("<%d>", __LINE__);
    }
  }
  VERB("<%d>", __LINE__);
  DumpStats();
}

void Break() { printf("\n@ Break @\n"); }

byte FieldOffset(word obj, byte member_name_isn) {
  word cls = ChainGetNth(ClassList, ocls(obj));
  if (!cls) return INF;
  word member_name = ChainGetNth(InternList, member_name_isn);
  // SayStr(member_name);
  word p = Class_fieldList(cls);

  struct ChainIterator iter;
  ChainIterStart(p, &iter);
  for (int i = 0; ChainIterMore(p, &iter); i++) {
    word e = ChainIterNext(p, &iter);
    if (e == member_name) {
      return (byte)i + (byte)i;
    }
  }
  // Not found.
  return INF;
}

word MemberGet(word obj, byte member_name_isn) {
  byte off = FieldOffset(obj, member_name_isn);
  assert(off != INF);
  return ogetw(obj + off);
}

void MemberPut(word obj, byte member_name_isn, word value) {
  byte off = FieldOffset(obj, member_name_isn);
  assert(off != INF);
  oputw(obj + off, value);
}

word ArgGet(byte i) {
  word old_fp = Frame_prev_frame(fp);
  int old_sp = Frame_prev_sp(fp);
  return ogetw(old_fp + (byte)old_sp + 2 * (i + 1));
}

word FindMethForObj(word obj, byte meth_isn) {
  word cls = ChainGetNth(ClassList, ocls(obj));
  SayStr(Class_className(cls));
  word dict = Class_methDict(cls);
  word meth_name = ChainGetNth(InternList, meth_isn);
  SayStr(meth_name);
  word meth = ChainDictGet(dict, meth_name);
  assert(meth);
  return meth;
}

word SingletonStr(byte ch) {
  word x = NewStr(0, 6, 1);
  Str_bytes_Put(x, x);
  Str_single_Put(x, (word)ch << 7);
  return x;
}
