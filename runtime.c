#include "runtime.h"

#include "_generated_proto.h"
#include "arith.h"
#include "chain.h"
#include "data.h"
#include "defs.h"
#include "octet.h"
#include "pb2.h"
#include "readbuf.h"

// GC Roots:
word RootForMain;  // For main() to use.

word Builtins;
word GlobalDict;  // todo: Modules.
word InternList;
word ClassList;
word MessageList;
word DunderInitStr;
byte DunderInitIsn;

byte* codes;     // ????
word codes_obj;  // ????

word function;
word ip;  // Stores in Frame as ip - function.
word fp;
word sp;  // Stores in Frame as sp - fp.

#if unix
#define VERB \
  if (true) printf
#else
#define VERB \
  if (false) printf
#endif

byte Hex(byte b) {
  b = b & 15;
  return (b < 10) ? '0' + b : 'a' + b - 10;
}
void SayChain(word p) {
#if unix
  struct ChainIterator iter;
  ChainIterStart(p, &iter);
  for (int i = 0; ChainIterMore(p, &iter); i++) {
    word e = ChainIterNext(p, &iter);
    printf(" [[%d]] ", i);
    SayObj(e, 2);
  }
  fflush(stdout);
#endif
}
void SayStr(word p) {
#if unix
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
#endif
}
void SayObj(word p, byte level) {
#if unix
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
        if (cls == C_Chain || cls == C_List || cls == C_Tuple ||
            cls == C_Dict) {
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
#endif
}

void SayStack() {
#if unix
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
#endif
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
  return true;

FALSE:
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
#if 0
  // fp0, the partial ur-frame.
  word fp0 = oalloc(32, C_Frame);
  // Put fn on the stack, to call with no args.
  byte cap0 = ocap(fp0);
  oputw(fp0 + cap0 - 2, fn);
  // XXX do I really need fp0?
#else
  word fp0 = 0;
#endif

  fp = oalloc(32, C_Frame);
  byte cap = ocap(fp);
  Frame_prev_frame_Put(fp, fp0);
  Frame_function_Put(fp, fn);
  Frame_nargs_Put(fp, 0);
  Frame_prev_sp_Put(fp, cap - 2);
  Frame_prev_ip_Put(fp, 0);
  sp = fp + ocap(fp);
  function = fn;
  ip = function + BC_HEADER_SIZE;

  RunLoop();
  printf("\n[[[ Finished RunLoop ]]]\n");
}

void RunLoop() {
  while (1) {
#ifdef CAREFUL
    VERB("\n");
    assert(sp >= fp + Frame_Size - 2);
    assert(sp <= fp + ocap(fp));
    SayStack();  // ===========
    assert(ip >= function + BC_HEADER_SIZE);
    assert(ip < function + INF);
#endif
    byte opcode = ogetb(ip);
#ifdef CAREFUL
    assert(opcode < CodeNames_SIZE);
    VERB("::::: f=%d ip~%d opcode=%d ((( %s ))) args=%d,%d fp=%d sp~%d\n",
         function, ip - function, opcode, CodeNames[opcode], ogetb(ip + 1),
         ogetb(ip + 2), fp, (sp - fp) >> 1);

    // SWITCH:
    assert(fp);
    assert(function);
    assert(sp > fp);
    assert(ip > function);
    assert(sp <= fp + ocap(fp));
    assert(ip < function + ocap(function));
#endif
    ip++;
    switch (opcode) {
#define PRIM_PART 3
#include "_generated_prim.h"
#undef PRIM_PART
      default:
        opanic(240);
    }  // end switch
  }    // next ip
}

void RunBuiltin(byte builtin_num) {
  switch (builtin_num) {
#define PRIM_PART 4
#include "_generated_prim.h"
#undef PRIM_PART
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
        DictPut(GlobalDict, s, None);
        g_num = 1 + DictWhatNth(GlobalDict, s);
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
        DictPut(dict, name_str, bc);
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
    SayObj(DunderInitStr, 3);
    osay(DunderInitStr);
    word init_meth = DictGet(meth_dict, DunderInitStr);
    SayObj(init_meth, 3);
    osay(init_meth);
    byte num_args_to_ctor = 0;
    if (init_meth) {
      assert(ocls(init_meth) == C_Bytecodes);
      num_args_to_ctor = ogetb(init_meth + BC_NUM_ARGS) - 1;
    }

    // Hand craft the constructor.
    word ctor = oalloc(16, C_Bytecodes);

    Bytecodes_flex_AtPut(ctor, 0, num_args_to_ctor);
    byte step = 3;
    Bytecodes_flex_AtPut(ctor, step++, INF);
    Bytecodes_flex_AtPut(ctor, step++, INF);
    Bytecodes_flex_AtPut(ctor, step++, INF);

    Bytecodes_flex_AtPut(ctor, step++, BC_Construct);
    Bytecodes_flex_AtPut(ctor, step++, class_num);
    Bytecodes_flex_AtPut(ctor, step++, num_args_to_ctor);  // less self.

    Bytecodes_flex_AtPut(ctor, step++, BC_Return);  // new obj

    DictPut(GlobalDict, name_str, ctor);
  }

  pb_next(bp);  // consume the 0 tag.
}

void SlurpModule(struct ReadBuf* bp, word* bc_out) {
  word bc;
  word ilist = NewList();
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
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
  omark(Builtins);
  omark(GlobalDict);
  omark(InternList);
  omark(ClassList);
  omark(MessageList);
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
void Directory() {
#if unix
  FOR_EACH(i, item, Builtins) DO printf(";; Builtin[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE

      FOR_EACH(i, item, GlobalDict) DO printf(";; GlobalDict[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE

      FOR_EACH(i, item, InternList) DO printf(";; InternList[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE

      FOR_EACH(i, item, ClassList) DO printf(";; ClassList[%d] :: ", i);
  SayObj(item, 2);
  if (item) {
    osay(item);
    printf("   num: %d\n", Class_classNum(item));
    printf("   name: ");
    SayObj(Class_methDict(item), 3);
    printf("   fieldList: ");
    SayObj(Class_fieldList(item), 3);
  }
  printf("End Of Class\n");
  DONE

      FOR_EACH(i, item, MessageList) DO printf(";; MessageList[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE
#endif
}
void RuntimeInit() {
  Builtins = NewList();
  GlobalDict = NewDict();  // todo: Modules.
  InternList = NewList();
  ClassList = NewList();
  MessageList = NewList();

  DunderInitIsn = InternString(NewStrCopyFromC("__init__"));
  DunderInitStr = ChainGetNth(InternList, DunderInitIsn);

  // Reserve builtin class slots, with None.
  ChainAppend(ClassList, None);  // unused 0.
  DumpStats();
  for (byte i = 1; i < ClassNames_SIZE; i++) {
    word cls = oalloc(Class_Size, C_Class);
    Class_classNum_Put(cls, i);
    const char* cstr = 2 /* skip `C_` */ + ClassNames[i];
    word name = NewStrCopyFromC(cstr);
    word name_isn = InternString(name);
    name = ChainGetNth(InternList, name_isn);
    Class_className_Put(cls, name);
    word d = NewDict();
    Class_methDict_Put(cls, d);
    ChainAppend(ClassList, cls);
  }
  for (byte i = 0; i < MessageNames_SIZE; i++) {
    word name = NewStrCopyFromC(MessageNames[i]);
    word name_isn = InternString(name);
    ChainAppend(MessageList, ChainGetNth(InternList, name_isn));
  }
  {
    byte i = 0;
    for (const byte* p = Prims; *p; p += 2, ++i) {
      word cls = ChainGetNth(ClassList, p[0]);
      SayObj(cls, 3);
      word meth_list = Class_methDict(cls);
      word message = ChainGetNth(MessageList, p[1]);
      ChainAppend(meth_list, message);
      ChainAppend(meth_list, FROM_INT(i));
    }
  }

  DumpStats();
}

void Break() {
#if unix
  printf("\n@ Break (((\n");
  printf("  fp=%d sp-fp=%d fn=%d ip-fn=%d\n", fp, sp - fp, function,
         ip - function);
  for (word p = fp; p; p = Frame_prev_frame(p)) {
    SayObj(p, 2);
    printf("  prev(fp=%d,sp=%d,ip=%d) fn=%d nargs=%d\n", Frame_prev_frame(p),
           Frame_prev_sp(p), Frame_prev_ip(p), Frame_function(p),
           Frame_nargs(p));
    osay(p);
  }
  printf("\n@ Break )))\n");
#endif
}

byte FieldOffset(word obj, byte member_name_isn) {
  word cls = ChainGetNth(ClassList, ocls(obj));
  if (!cls) return INF;
  word member_name = ChainGetNth(InternList, member_name_isn);
  VERB("Field Offset of ");
  SayStr(member_name);
  word p = Class_fieldList(cls);

  struct ChainIterator iter;
  ChainIterStart(p, &iter);
  for (int i = 0; ChainIterMore(p, &iter); i++) {
    word e = ChainIterNext(p, &iter);
    if (e == member_name) {
      VERB("Field %d off %d\n", i, i + i);
      return (byte)i + (byte)i;
    }
  }
  VERB("Not Found.\n");
  // Not found.
  return INF;
}

word MemberGet(word obj, byte member_name_isn) {
  byte off = FieldOffset(obj, member_name_isn);
  assert(off != INF);
  word addr = obj + off;
  word z = ogetw(addr);
  VERB("addr %d MemberGet->%d\n", addr, z);
  return z;
}

void MemberPut(word obj, byte member_name_isn, word value) {
  byte off = FieldOffset(obj, member_name_isn);
  assert(off != INF);
  word addr = obj + off;
  VERB("addr %d MemberPut<-%d\n", addr, value);
  oputw(addr, value);
}

word ArgGet(byte i) {
  VERB("ArgGet(%d): ", i);
  word old_fp = Frame_prev_frame(fp);
  VERB("fp: %d old_fp %d ", fp, old_fp);
  int old_sp = old_fp + Frame_prev_sp(fp);
  VERB("prev_sp: %d old_sp %d ", Frame_prev_sp(fp), old_sp);
  word addr = old_sp + i + i;
  word z = ogetw(addr);
  VERB("addr: %d z: %d\n", addr, z);
  return z;
}

word FindMethForObjOrNull(word obj, byte meth_isn) {
  word cls = ChainGetNth(ClassList, ocls(obj));
  SayStr(Class_className(cls));
  word dict = Class_methDict(cls);
  word meth_name = ChainGetNth(InternList, meth_isn);
  SayStr(meth_name);
  word meth = DictGet(dict, meth_name);
  return meth;
}
word SingletonStr(byte ch) {
  word x = NewStr(0, 6, 1);
  Str_bytes_Put(x, x);
  Str_single_Put(x, (word)ch << 7);
  return x;
}

void Construct(byte cls_num, byte nargs /*less self */) {
  // Push nargs args from caller's frame onto our stack.
  word old_fp = Frame_prev_frame(fp);
  word old_sp = old_fp + Frame_prev_sp(fp);
  for (byte i = 0; i < nargs; i++) {
    word p = old_sp + ((nargs - i - 1) << 1);
    word tmp_arg = ogetw(p);
    sp -= 2;
    oputw(sp, tmp_arg);
  }

  // Create the object and push self to our stack.
  byte size = 32;
  word obj = oalloc(size, cls_num);
  sp -= 2;
  oputw(sp, obj);

  word meth = FindMethForObjOrNull(obj, DunderInitIsn);
  if (meth) {
    byte want = ogetb(meth + BC_NUM_ARGS);  // counts self.
    assert2(want == 1 + nargs, "__init__ got %d args, wants %d", 1 + nargs,
            want);
    CallMeth(DunderInitIsn, nargs + 1);  // add 1 for self.
  } else {
    assert(!nargs);
  }
}

void Call(byte nargs, word fn) {
  Break();
  if (fn & 1) {  // If odd, is an integer.
    RunBuiltin((byte)TO_INT(fn));
    return;
  }
  assert(ocls(fn) == C_Bytecodes);

  word old_fp = fp;
  fp = oalloc(32, C_Frame);

  Frame_prev_frame_Put(fp, old_fp);
  Frame_function_Put(fp, fn);
  Frame_nargs_Put(fp, nargs);
  Frame_prev_sp_Put(fp, sp - old_fp);
  Frame_prev_ip_Put(fp, ip - function);

  function = fn;
  Frame_function_Put(fp, function);
  assert2(ocls(function) == C_Bytecodes, "func got cls %d, want %d",
          ocls(function), C_Bytecodes);
  assert2(ogetb(function + BC_NUM_ARGS) == nargs, "func got %d args, want %d",
          nargs, ogetb(function + BC_NUM_ARGS));
  sp = fp + ocap(fp);
  ip = function + BC_HEADER_SIZE;
}

void CallMeth(byte meth_isn, byte nargs /* with self */) {
  word self = ogetw(sp);
  word fn = FindMethForObjOrNull(self, meth_isn);
  assert2(fn, "meth %d not found on self %d", meth_isn, self);

  Call(nargs, fn);
}

// Return true to stop.
bool ReturnPerhapsStop(word retval) {
  Break();
  word old_fp = Frame_prev_frame(fp);
  if (!old_fp) {
    // Stop when no previous frame.
    printf("\n[[[ Returning from urframe ]]]\n");
    return true;  // Do stop.
  }

  word nargs = Frame_nargs(fp);
  int old_sp = old_fp + Frame_prev_sp(fp);

  function = Frame_function(old_fp);
  ip = function + Frame_prev_ip(fp);
  fp = old_fp;
  // pop args, push retval.
  // actually function was there to, but leave one slot on stack for retval.
  sp = old_sp + (nargs << 1) - 2;  // -2 for retval.
  for (word p = old_sp; p < sp; p += 2) {
    oputw(p, 0xDEAD);
  }
  oputw(sp, retval);
  return false;  // don't stop.
}
void DoTry(byte catch_loc) {
  word try = oalloc(Try_Size, C_Try);
  Try_catch_loc_Put(try, catch_loc);

  // Add link to front of tries.
  word next = Frame_tries(fp);
  Try_next_Put(try, next);
  Frame_tries_Put(fp, try);
}
void DoCatch(byte end_catch_loc) {
  // Called at the end of a try.
  // Consulted on a Catch, which starts at the next opcode.

  // Remove one Try block from the `tries` chain.
  word try = Frame_tries(fp);
  word next = Try_next(try);
  Frame_tries_Put(fp, next);
  ip = function + end_catch_loc;
}
bool DoRaisePossiblyQuit(byte ex) {
  for (word p = fp; p; p = Frame_prev_frame(p)) {
    word try = Frame_tries(p);
    if (try) {
      fp = p;

      // Unlink the Try record.
      word next = Try_next(try);
      Frame_tries_Put(fp, next);

      // Set execution state to Catch clause.
      function = Frame_function(fp);
      sp = fp + ocap(fp);  // empty stack.
      // catch_loc is the 3-byte "Catch" instruction,
      // which we do not want to execute,
      // but we do want the last byte, the local var num.
      ip = function + Try_catch_loc(try) + 3;
      byte local_var_num = ogetb(ip - 1);
      Frame_flex_AtPut(fp, local_var_num, ex);
      return false;
    }
  }
  printf("\nUncaught Exception: ");
  SayObj(ex, 3);
  osay(ex);
  return true;
}
