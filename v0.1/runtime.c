#include "runtime.h"

#include "_generated_proto.h"
#include "arith.h"
#include "chain.h"
#include "data.h"
#include "defs.h"
#include "octet.h"
#include "osetjmp.h"
#include "pb2.h"
#include "readbuf.h"

// GC Roots:
word RootForMain;  // For main() to use.
word LoopAllocRoot;  // For DictItems, etc.

word Builtins;
word GlobalDict;  // todo: Modules.
word InternList;
word ClassList;
word MessageList;
word DunderInitStr;
word DunderInitIsn;
word DunderIterIsn;
word NextIsn;
word StopIterationStr;

#define SIGNALS 0
#if SIGNALS
word SignalStr;
#endif

byte* codes;     // ????
word codes_obj;  // ????

word function;
word ip;  // Stores in Frame as ip - function.
word fp;
word sp;  // Stores in Frame as sp - fp.
byte signalled;

#if unix
#define VERB \
  if (true) printf
#else
#define VERB \
  if (false) printf
#endif


#if unix
void FatalCoreDump() {
  printf("*** FatalCoreDump\n");
  fprintf(stderr, "*** FatalCoreDump\n");
  assert(0);
}
void PrintK() {
  printf("*** PrintK\n");
  fprintf(stderr, "*** PrintK\n");
}
void PrintK5(word a, word b, word c, word d, word e) {
  printf("*** PrintK %x %x %x %x %x\n", a, b, c, d, e);
  fprintf(stderr, "*** PrintK %x %x %x %x %x\n", a, b, c, d, e);
}
#else
void FatalCoreDump() {
  asm {
    swi
    fcb 100  ; FatalCoreDump HyperOp
  }
  return;
}

void PrintK() {
  asm {
    swi
    fcb 101  ; PrintK HyperOp
  }
  return;
}
void PrintK5(word a, word b, word c, word d, word e) {
  asm {
    swi
    fcb 101  ; PrintK HyperOp
  }
  return;
}
#endif

byte Hex(byte b) {
  b = b & 15;
  return (b < 10) ? '0' + b : 'a' + b - 10;
}
void SayTrain(word train) {
#if unix
  struct TrainIterator iter;
  TrainIterStart(train, &iter);
  for (int i = 0; TrainIterMore(&iter); i++) {
    word e = TrainIterNext(&iter);
    printf(" [[%d]] ", i);
    SayObj(e, 2);
  }
  fflush(stdout);
#endif
}
void SimplePrint(word p) {
  if (p & 1) {
    printf("%d ", TO_INT(p));
  } else if (p == 0) {
    printf("None ");
  } else if (ocls(p) == C_Str) {
    byte n = StrLen(p);
    for (byte i = 0; i < n; i++) {
      printf("%c", StrAt(p, i));
    }
    printf(" ");
  } else {
    printf("$%04x:%d:%d$ ", p, ocap(p), ocls(p));
  }
  fflush(stdout);
}
void ShowStr(word p) {
  assert(ocls(p) == C_Str);
  printf("'");
  byte n = StrLen(p);
  for (byte i = 0; i < n; i++) {
    byte ch = StrAt(p, i);
    if (' ' <= ch && ch <= '~') {
      printf("%c", ch);
    } else {
      printf("?");
    }
  }
  printf("'");
  fflush(stdout);
}
void SayStr(word p) {
#if unix
  ShowStr(p);
#endif
}
void ShowL(word p, byte level) {
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
      ShowStr(p);
    } else {
      if (cls < ClassNames_SIZE) {
        clsname = ClassNames[cls];
        printf("cls %s: cap=%d", clsname, ocap(p));
        if (level < 2) return;
        if (cls == C_Train || cls == C_List || cls == C_Tuple ||
            cls == C_Dict) {
          struct TrainIterator it;
          TrainIterStart(p, &it);
          printf(" list: (len2=%d, next=$%x)\n", Train_len2(p), Train_next(p));
          for (byte i = 0; TrainIterMore(&it); ++i) {
            word e = TrainIterNext(&it);
            printf("\n  [%d]: ", i);
            ShowL(e, level - 1);
          }
          printf("\n");
        } else if (cls == C_Dict) {
          struct TrainIterator it;
          TrainIterStart(p, &it);
          printf(" dict:\n");
          for (byte i = 0; TrainIterMore(&it); ++i) {
            word k = TrainIterNext(&it);
            word v = TrainIterNext(&it);
            printf("\n  [[[");
            ShowL(k, level - 1);
            printf("]]] = ");
            ShowL(v, level - 1);
          }
          printf("\n");
        }
      } else if (cls < TrainLen2(ClassList)) {
        word clsobj = TrainGetNth(ClassList, cls);
        if (clsobj) {
          printf("cls=");
          ShowStr(Class_className(clsobj));
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
void Show(word p) { ShowL(p, 2); }
void SayObj(word p, byte level) {
#if unix
  ShowL(p, level);
#endif
}

#if unix
void SayPyStack() {
  byte cap = ocap(fp);
  int i = 0;
  for (word p = sp; p < fp + cap; p += 2) {
    word x = ogetw(p);
    printf(":::::  [%d]  ", p);
    Show(x);
    printf("\n");
    ++i;
  }
  fflush(stdout);
}
#endif

#if 0  // unused for now
word NewBuf() {
  word buf = Buf_NEW();
  word guts = oalloc(254, C_Bytes);  // XXX this is huge
  Buf_bytes_Put(buf, guts);
  return buf;
}
#endif
word StrFromC(const char* s) {
  int s_len = strlen(s);
  assert(s_len <= 253);
  byte len = (byte)s_len;
  word str = oalloc(len+2, C_Str);  // 2 = 1(len) + 1(NUL)
  oputb(str, len); // len
  for (byte i = 0; i < len; i++) {
    oputb(str + 1 + i, s[i]);
  }
  oputb(str + 1 + len, 0); // NUL termination.
  return str;
}

bool Truth(word a) {
    if (a == 0) return false;       // None
    if (a == 0x0001) return false;  // Small int 0.

    // TODO: 0 was crashing inside ocls(a) but passed ovaliaddr(a)????

  if (ovalidaddr(a)) {
    switch (ocls(a)) {
      case C_Train:
      case C_List:
      case C_Dict:
        return (TrainLen2(a) > 0);
      case C_Str:
        return (StrLen(a) > 0);
    }
  } else {
    if (a == 0) return false;       // None
    if (a == 0x0001) return false;  // Small int 0.
  }
  return true;
}

bool StrEqual(word a, word b) {
  byte len_a = StrLen(a);
  assert(0 <= len_a && len_a <= 253);

  // i iterates the length, then the contents.
  for (byte i = 0; i <= len_a; i++) {
    if (ogetb(a + i) != ogetb(b + i)) {
      return false;
    }
  }
  return true;
}

bool Equal(word a, word b) {
  if (a == b) {
    return true;
  }
  if ((byte)a & 1) return false;
  if ((byte)b & 1) return false;
  if (ovalidaddr(a) && ovalidaddr(b)) {
    if (ocls(a) == C_Str && ocls(b) == C_Str) {
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
  fp = oalloc(FRAME_SIZE, C_Frame);
  byte cap = ocap(fp);
  Frame_prev_frame_Put(fp, None);
  Frame_function_Put(fp, fn);
  Frame_nargs_Put(fp, 0);
  Frame_prev_sp_Put(fp, cap - 2);
  Frame_prev_ip_Put(fp, 0);
  sp = fp + ocap(fp);
  function = fn;
  ip = function + BC_HEADER_SIZE;

  ocheckall();
  RunLoop();
  ocheckall();
  printf(" [unloop] ");
  fp = function = None;
  sp = ip = 0;
#if 0
  FatalCoreDump();
#endif
}

#if SIGNALS
#if !unix
asm void Intercept() {
  asm {
    stb signalled
    rti
  }
}
#endif

void SetIntercept() {
#if !unix
  asm {
    pshs y,u
    ldx #_Intercept
    SWI2
    FCB 9
    puls y,u
  }
#endif
}
#endif

ojmp_buf run_loop_jmp_buf;

void RunLoop() {
#if SIGNALS
  SetIntercept();
#endif
  byte message = (byte)osetjmp(run_loop_jmp_buf);
  if (message == FINISH) return;

RUN_LOOP:
#if SIGNALS
  if (signalled) {
    printf(" <SIG%d> ", signalled);
    signalled = 0;
    if (fp) {
      Raise(SignalStr);
      goto RUN_LOOP;
    } else {
      return;
    }
  }
#endif

#if CAREFUL || unix
  VERB("\n");
  assert(fp);
  assert(function);
  assert(sp >= fp + Frame_Size - 2);
  assert(sp <= fp + ocap(fp));
#if unix
  SayPyStack();  // ===========
#endif
  assert(ip >= function + BC_HEADER_SIZE);
  assert(ip < function + ocap(function));
#endif
  byte opcode = ogetb(ip);
  // PrintK();
  VERB("%d:", opcode);
#if CAREFUL || unix
  assert(opcode < CodeNames_SIZE);
#endif
  VERB("%s> ", CodeNames[opcode]);
#if CAREFUL || unix
  assert(opcode < CodeNames_SIZE);
#endif
#if unix
  printf("::::: f=%d ip~%d opcode=%d ((( %s ))) args=%d,%d fp=%d sp~%d\n",
       function, ip - function, opcode, CodeNames[opcode], ogetb(ip + 1),
       ogetb(ip + 2), fp, (sp - fp) >> 1);
#endif

  ip++;
  switch (opcode) {
#define PRIM_PART 3
#include "_generated_prim.h"
#undef PRIM_PART
  }  // end switch
  printf("BAD opcode=%d function=%d ip=%d fp=%d sp=%d\n", opcode, function, ip, fp,
         sp);
  opanic(240);
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
  struct TrainIterator it;
  TrainIterStart(InternList, &it);
  byte i = 0;
  while (TrainIterMore(&it)) {
    word s = TrainIterNext(&it);
    if (StrEqual(s, str)) return i;
    ++i;
  }
  TrainAppend(InternList, str);
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

        word str = oalloc(bytes_len+2, C_Str);
        oputb(str, bytes_len); // len
        omemcpy(str+1, bytes, bytes_len);  // guts
        oputb(str+1+bytes_len, 0);  // NUL

        i_num = InternString(str);
        assert(i_num < INF);
        TrainAppend(ilist, Q(i_num));
      } break;
      case InternPack_patch: {
        word offset = pb_int(bp);
        assert(offset < ocap(bc));
        assert(i_num < INF);
        oputb(bc + offset, i_num);
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
        byte i_num = (byte)N(TrainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // TODO -- append to GlobalList.
        // Get the i_numth interned string.
        word s = TrainGetNth(InternList, i_num);
        assert(s);
        // Create a global slot for that string, if not already there.
        if (!DictAddr(GlobalDict, s)) {
          DictPut(GlobalDict, s, None);
        }
        g_num = 1 + DictWhatNth(GlobalDict, s);
        assert(g_num != INF);
        // printf("\nS_GLOBAL: ith=%d i_num=%d g_num=%d s=", ith, i_num, g_num);
        // Show(s);
        // printf("\n");
        // osaylabel(bc, "GlobalPack_name_i", ith);
      } break;
      case GlobalPack_patch: {
        word offset = pb_int(bp);
        assert(offset < ocap(bc));
        oputb(bc + offset, g_num);
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
  printf("f");
  word name_str;
  word ith;
  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case FuncPack_name_i: {
        ith = pb_int(bp);
        byte i_num = (byte)N(TrainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // Get the i_numth interned string.
        name_str = TrainGetNth(InternList, i_num);
        assert(name_str);
        // printf("SLURPING FUNC: <<<");
        // SayObj(name_str, 2);
        // printf(">>>\n");
      } break;
      case FuncPack_pack: {
        word bc;
        pb_next(bp);
        SlurpCodePack(bp, &bc);
        DictPut(dict, name_str, bc);
        // printf("\nS_FuncPack pack:");
        // Show(name_str);
        // Show(bc);
        // Show(dict);
        // osaylabel(bc, "FuncPack_name_i", ith);
      } break;
      default:
        opanic(97);
    }
  }
  pb_next(bp);  // consume the 0 tag.
}
void SlurpClassPack(struct ReadBuf* bp, word ilist) {
  printf("c");
  word class_num = ListLen(ClassList);
  word name_str;
  word ith;
  word field_list = NewList();
  word meth_dict = NewDict();

  word cls = oalloc(Class_Size, C_Class);
  Class_classNum_Put(cls, class_num);
  Class_fieldList_Put(cls, field_list);
  Class_methDict_Put(cls, meth_dict);
  TrainAppend(ClassList, cls);

  for (byte tag = pb_current(bp); tag; tag = pb_current(bp)) {
    switch (tag) {
      case ClassPack_name_i: {
        ith = pb_int(bp);
        byte i_num = (byte)N(TrainGetNth(ilist, (byte)ith));
        assert(i_num < INF);
        // Get the i_numth interned string.
        name_str = TrainGetNth(InternList, i_num);
        assert(name_str);
        // printf("SLURPING CLASS: [[[");
        // SayObj(name_str, 2);
        Class_className_Put(cls, name_str);
        // SayObj(cls, 2);
    // printf("   classNum: %d\n", Class_classNum(cls));
    // printf("   className: ");
    // Show(Class_className(cls));
    // printf("   methDict: ");
    // SayObj(Class_methDict(cls), 3);
    // printf("   fieldList: ");
    // SayObj(Class_fieldList(cls), 3);
        // printf("]]]\n");
      } break;
      case ClassPack_field_i: {  // For a field.
        word field_ith = pb_int(bp);
        byte field_i_num = (byte)N(TrainGetNth(ilist, (byte)field_ith));
        assert(field_i_num < INF);
        word field_str = TrainGetNth(InternList, field_i_num);
        assert(field_str);
        // printf("SLURPING FIELD <<<");
        // SayObj(field_str, 2);
        // printf(">>>\n");
        TrainAppend(field_list, field_str);
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
    // Find the __init__ method, for creating the ctor.
    word init_meth = DictGetOrDefault(meth_dict, DunderInitStr, None);

    // Default ctors take no args.
    byte num_args_to_ctor = 0;
    if (init_meth) {
      // Find out how many args the ctor needs from __init__.
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
  printf("m");
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
  omark(LoopAllocRoot);
  omark(function);
  omark(fp);
  // data.c:
  omark(Stdin);
  omark(Stdout);
  omark(Stderr);
}
void Directory() {
#if unix
  FOR_EACH(i, item, Builtins) DO printf(";; Builtin[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE
#endif

#if unix
      FOR_EACH(i, item, GlobalDict) DO printf(";; GlobalDict[%d] :: ", i);
  Show(item);
  // osay(item);
  printf("\n");
  DONE

      FOR_EACH(i, item, InternList) DO printf(";; InternList[%d] :: ", i);
  Show(item);
  // osay(item);
  printf("\n");
  DONE

      FOR_EACH(i, item, ClassList) DO printf(";; ClassList[%d] :: ", i);
  SayObj(item, 2);
  if (item) {
    osay(item);
    printf("   classNum: %d\n", Class_classNum(item));
    printf("   className: ");
    Show(Class_className(item));
    printf("   methDict: ");
    SayObj(Class_methDict(item), 3);
    printf("   fieldList: ");
    SayObj(Class_fieldList(item), 3);
  }
  printf("End Of Class\n");
  DONE
#endif

#if unix

      FOR_EACH(i, item, MessageList) DO printf(";; MessageList[%d] :: ", i);
  SayObj(item, 2);
  osay(item);
  DONE
#endif
}
void RuntimeInit() {
  ocheckall();
  Builtins = NewList();
  GlobalDict = NewDict();  // todo: Modules.
  InternList = NewList();
  ClassList = NewList();
  MessageList = NewList();

  DunderInitIsn = InternString(StrFromC("__init__"));
  DunderInitStr = TrainGetNth(InternList, DunderInitIsn);
  DunderIterIsn = InternString(StrFromC("__iter__"));
  NextIsn = InternString(StrFromC("next"));
  StopIterationStr = TrainGetNth(InternList, InternString(StrFromC("StopIteration")));
#if SIGNALS
  SignalStr = TrainGetNth(InternList, InternString(StrFromC("SIGNAL")));
#endif

  // Reserve builtin class slots, with None.
  TrainAppend(ClassList, None);  // unused 0.
  // odumpsummary();
  for (byte i = 1; i < ClassNames_SIZE; i++) {
    word cls = oalloc(Class_Size, C_Class);
    Class_classNum_Put(cls, i);
    const char* cstr = ClassNames[i];
    word name = StrFromC(cstr);
    word name_isn = InternString(name);
    name = TrainGetNth(InternList, name_isn);
    Class_className_Put(cls, name);
    word d = NewDict();
    Class_methDict_Put(cls, d);
    TrainAppend(ClassList, cls);
  }
  for (byte i = 0; i < MessageNames_SIZE; i++) {
    word name = StrFromC(MessageNames[i]);
    word name_isn = InternString(name);
    TrainAppend(MessageList, TrainGetNth(InternList, name_isn));
  }
  {
    byte i = 0;
    for (const byte* p = Prims; *p; p += 2, ++i) {
      word cls = TrainGetNth(ClassList, p[0]);
      SayObj(cls, 3);
      word meth_list = Class_methDict(cls);
      word message = TrainGetNth(MessageList, p[1]);
      TrainAppend(meth_list, message);
      TrainAppend(meth_list, FROM_INT(i));
      // printf(
      //"PRIMS: %d %d -> cls %d meth_list %d message %d i %d FROM_INT(i) "
      //"%d\n",
      // p[0], p[1], cls, meth_list, message, i, FROM_INT(i));
    }
  }

  // odumpsummary();
  ocheckall();
  ogc();
  ocheckall();
  // odumpsummary();

#if 0
  {
    printf("################ Read (((\n");
    word file = PyOpenFile(
#if unix
        StrFromC("/etc/fstab"),
#else
        StrFromC("STARTUP"),
#endif
        StrFromC("r"));
    assert(file);
    osay(file);
    while (1) {
      word s = FileReadLineToNewBuf(file);
      osay(s);
      if (!s) break;
    }
    printf("################ Done )))\n");
  }
#endif
}

void Break(const char* why) {
#if unix
  printf("\n@ Break `%s'(((\n", why);
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
#else
  VERB(" <%s> ", why);
#endif
}

byte FieldOffset(word obj, byte member_name_isn) {
  word cls = TrainGetNth(ClassList, ocls(obj));
  if (!cls) return INF;
  word member_name = TrainGetNth(InternList, member_name_isn);
  VERB("Field Offset of ");
  SayStr(member_name);
  word p = Class_fieldList(cls);

  struct TrainIterator iter;
  TrainIterStart(p, &iter);
  for (int i = 0; TrainIterMore(&iter); i++) {
    word e = TrainIterNext(&iter);
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
  if (off == INF) RaiseC("bad_member_get");

  word addr = obj + off;
  word z = ogetw(addr);
  VERB("addr %d MemberGet->%d\n", addr, z);
  return z;
}

void MemberPut(word obj, byte member_name_isn, word value) {
  byte off = FieldOffset(obj, member_name_isn);
  if (off == INF) RaiseC("bad_member_put");

  word addr = obj + off;
  VERB("addr %d MemberPut<-%d\n", addr, value);
  oputw(addr, value);
}

word ArgGet(byte i) {
  word old_fp = Frame_prev_frame(fp);
  int old_sp = old_fp + Frame_prev_sp(fp);
  word addr = old_sp + i + i;
  word z = ogetw(addr);
  return z;
}
void ArgPut(byte i, word a) {
  word old_fp = Frame_prev_frame(fp);
  int old_sp = old_fp + Frame_prev_sp(fp);
  word addr = old_sp + i + i;
  oputw(addr, a);
}

word FindMethForObjOrNull(word obj, byte meth_isn) {
  word cls = TrainGetNth(ClassList, ocls(obj));
  SayStr(Class_className(cls));
  word dict = Class_methDict(cls);
  word meth_name = TrainGetNth(InternList, meth_isn);
  SayStr(meth_name);
  word meth = DictGetOrDefault(dict, meth_name, None);
  return meth;
}
word SingletonStr(byte ch) {
  word str = oalloc(3, C_Str);
  oputb(str, 1); // len
  oputb(str+1, ch); // guts
  oputb(str+2, 0); // NUL
  return str;
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
  // TODO: correct object size.
  word cls = ListGetNth(ClassList, cls_num);
  assert(cls);
  word fields = Class_fieldList(cls);
  assert(fields);
  word flen = ListLen(fields);
  byte size = flen+flen;  /// OBJECT_SIZE;
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
#if 0
  if (fn == 0) {
    PrintK5(10, 20, 30, 40, 50);
  }
  // printf(" Call:#%x(%x)\n", fn, nargs);
#endif
  if (fn & 1) {  // If odd, is an integer.
    byte i = (byte)TO_INT(fn);
    Break("PRIM");
    RunBuiltin(i);
    return;
  }
  Break("CALL");
  if(ocls(fn) != C_Bytecodes) {
    printf("Call:");
    Show(fn);
  }
  CHECK(ocls(fn) == C_Bytecodes, "bad_fn_cls");
  if(ogetb(fn + BC_NUM_ARGS) != nargs) {
    printf("Call: want %d got %d nargs\n", ogetb(fn+BC_NUM_ARGS), nargs);
  }
  CHECK(ogetb(fn + BC_NUM_ARGS) == nargs, "bad_nargs");

  word old_fp = fp;
  fp = oalloc(FRAME_SIZE, C_Frame);

  Frame_prev_frame_Put(fp, old_fp);
  Frame_function_Put(fp, fn);
  Frame_nargs_Put(fp, nargs);
  Frame_prev_sp_Put(fp, sp - old_fp);
  Frame_prev_ip_Put(fp, ip - function);

  function = fn;
  Frame_function_Put(fp, function);
  sp = fp + ocap(fp);
  ip = function + BC_HEADER_SIZE;
  ocheckall();
}

void PleaseCallMeth0(byte meth_isn, word self) {
  PushSp(self);
  CallMeth(meth_isn, 1);
}

void CallMeth(byte meth_isn, byte nargs /* with self */) {
  word self = ogetw(sp);
  // printf(" CM:%x #%x(%x)\n", self, meth_isn, nargs);
  word fn = FindMethForObjOrNull(self, meth_isn);
  if (!fn) {
    Show(TrainGetNth(InternList, meth_isn));
    Show(self);
    osay(self);
  }
  assert2(fn, "meth %d not found on self %d", meth_isn, self);

  Call(nargs, fn);
}

void Return(word retval) {
  Break("RETURN");
  word old_fp = Frame_prev_frame(fp);
  if (!old_fp) {
    // Stop when no previous frame.
    printf(" [unframe] ");
    if (retval) {
      printf("RESULT: ");
      SimplePrint(retval);
      printf("\n");
    }

    olongjmp(run_loop_jmp_buf, FINISH);
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
}
#if 0
void MaybePrintDepth() {
  word nf=0, nt=0;
  printf("DEPTH: ");
  for (word p = fp; p; p = Frame_prev_frame(p)) {
    printf("[%d]", p);
    nf++;
    for (word t = Frame_tries(p); t; t = Try_next(t)) {
      printf("%d,", t);
      nt++;
    }
  }
  printf("  DEPTH: nf=%d nt=%d\n", nf, nt);
}
#endif
void DoFor() {
  word a = PopSp();
  word isn = DunderIterIsn;
  PleaseCallMeth0(isn, a);
}
void DoNext() {
  word a = PopSp();
  word isn = NextIsn;
  PleaseCallMeth0(isn, a);
}
void DoHandleStopIteration(byte end_while) {
  word ex = PopSp();
 if (!Compare(ex, StopIterationStr)) {
   // accept "StopIteration" and break the while loop.
   ip = function + end_while;
 } else {
   Raise(ex);  // re-raise the exception.
 }
}
void ShedTryBlocks(byte n) {
  assert(fp);
  for (byte i=0; i<n; i++) {
    word t = Frame_tries(fp);
    assert(t);
    Frame_tries_Put(fp, Try_next(t));
    ofree(t);
  }
}

void DoTry(byte catch_loc) {
  word try = oalloc(Try_Size, C_Try);
  Try_catch_loc_Put(try, catch_loc);

  // Add link to front of tries.
  word next = Frame_tries(fp);
  Try_next_Put(try, next);
  Frame_tries_Put(fp, try);
  // MaybePrintDepth();
}
void DoEndTry(byte end_catch_loc) {
  // Called at the end of a try.

  // Remove one Try block from the `tries` chain.
  word try = Frame_tries(fp);
  word next = Try_next(try);
  Frame_tries_Put(fp, next);
  ofree(try);
  ip = function + end_catch_loc;
}
void RaiseC(const char* msg) {
  // printf("\nRaiseC: %s\n", msg);
  Raise(StrFromC(msg));
}
void Raise(word ex) {
  assert(fp);  // Should be in a RunLoop
  Break("RAISE");
  for (word p = fp; p; p = Frame_prev_frame(p)) {
    word try = Frame_tries(p);
    // printf("frame %d tries=%d\n", p, try);
    if (try) {
      fp = p;

      // Unlink the Try record.
      word next = Try_next(try);
      Frame_tries_Put(fp, next);

      // Set execution state to Catch clause.
      function = Frame_function(fp);
      sp = fp + ocap(fp);  // empty stack.
      PushSp(ex);

      ip = function + Try_catch_loc(try);
      ofree(try);
      Break("LONGJMP");
      olongjmp(run_loop_jmp_buf, CONTINUE);
    }
  }
  printf("\nUncaught Exception: ");
  SayObj(ex, 3);
  osay(ex);
  assert(0);
}
void Implode(byte len, word chain) {
  for (word p = sp + ((word)len << 1) - 2; p >= sp; p -= 2) {
    TrainAppend(chain, ogetw(p));
  }
  sp += ((word)len << 1) - 2;  // -2 for new list.
  oputw(sp, chain);
}
int Len(word o) {
  int n;
  switch (ocls(o)) {
    case C_Pair:
      return 2;
    case C_Str:
      n = StrLen(o);
      break;
    case C_Tuple:
    case C_List:
      n = ListLen(o);
      break;
    case C_Dict:
      n = ListLen(o) >> 1;
      break;
    default:
      opanic(101);
  }
  return n;
}
void Explode(byte len) {
  word o = ogetw(sp);
  sp += 2;
  byte n = ToByte(Len(o));
  if (n != len) {
    RaiseC("explode_bad_len");
  }
  byte j = n;
  for (byte i = 0; i < n; i++) {
    j--;
    word x = GetItem(o, FROM_INT(j));
    sp -= 2;
    oputw(sp, x);
  }
}
word GetItem(word coll, word key) {
  word value;
  byte cls = ocls(coll);
  if (cls == C_Dict) {
      value = TrainMapGet(coll, key);
  } else {
      int i = TO_INT(key);
      assert(i>=0);
      assert(i<253);
      switch (cls) {
        case C_Str: {
          value = SingletonStr(StrAt(coll, i));
        } break;
        case C_Pair:
          assert(i<2);
          value = ogetw(coll+i+i);
          break;
        case C_Tuple:
        case C_List: {
          value = TrainGetNth(coll, i);
        } break;
        default:
          opanic(101);
      }
  }
  return value;
}
word PutItem(word coll, word key, word value) {
  switch (ocls(coll)) {
    case C_List: {
      int i = TO_INT(key);
      TrainPutNth(coll, i, value);
    } break;
    case C_Dict:
      TrainMapPut(coll, key, value);
      break;
    default:
      opanic(101);
  }
  return value;
}
word PopSp() {
  word z = ogetw(sp);
  oputw(sp, 0xDEAD);
  sp += 2;
  return z;
}
void PushSp(word a) {
  sp -= 2;
  oputw(sp, a);
}
