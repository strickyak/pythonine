# lex.py -- lexical tokenizer.

import re, sys
import _generated_proto as T # Tags.
import py_pb as P  # Protocol buffers.
E = sys.stderr

L_EOF = 0
L_INT = 1
L_STR = 2
L_IDENT = 3
L_MULTI = 4
L_PUNC = 5
L_EOL = 6

BytecodeNumbers = {}
def GetBytecodeNumbers():
  # Demo: BC_Print = 7,
  regexp = re.compile('BC_([A-Za-z0-9_]+) = ([0-9]+),')
  with open('_generated_core.h') as fd:
    for line in fd:
      m = regexp.match(line.strip())
      if m:
        BytecodeNumbers[m.group(1)] = int(m.group(2))
    
def IsDigit(c):
    return '0' <= c <= '9'
def IsAlfa(c):
    return 'A' <= c <= 'Z' or 'a' <= c <= 'z' or c=='_'
def IsWhite(c):
    return c <= ' '
def Show(a):
    print>>E, 'SHOW{ %s }' % [a]
    return a

class Lexer(object):
    def __init__(self, program):
        self.program = program
        self.i = 0

    def UnGetC(self):
          self.i -= 1
    def GetC(self):
            if self.i >= len(self.program):
                return ''
            z = self.program[self.i]
            self.i += 1
            return z

    def Next(self):
        c = self.GetC()
        if not c: return Show(( L_EOF, None ))
        while IsWhite(c):
            if c=='\n' or c=='\r': return Show(( L_EOL, None ))
            c = self.GetC()
        if IsDigit(c):
            x = 0
            while IsDigit(c):
                x = x*10 + ord(c) - 48
                c = self.GetC()
            self.UnGetC()
            return Show(( L_INT, x ))
        if IsAlfa(c):
            x = ''
            while IsAlfa(c) or IsDigit(c):
                x += c
                c = self.GetC()
            self.UnGetC()
            return Show(( L_IDENT, x ))
        if c in ['=', '!', '<', '>']:
            d = self.GetC()
            if d in ['=', '<', '>']:
              return Show(( L_MULTI, c+d ))
            else:
              self.UnGetC()
              # and fallthrough
        return Show(( L_PUNC, c ))

class Parser(object):
    def __init__(self, program):
        self.program = program
        self.lex = Lexer(program)
        self.Advance()

    def Advance(self):
        self.t, self.x = self.lex.Next()

    def ParsePrim(self):
        val = self.x
        if self.t == L_INT:
            self.Advance()
            return TInt(val)
        if self.t == L_STR:
            self.Advance()
            return TStr(val)
        if self.t == L_IDENT:
            self.Advance()
            return TIdent(val)
        raise Exception('bad prim: %s %s' % (self.t, val))
        
    def ParseProduct(self):
        p = self.ParsePrim()
        op = self.x
        while op=='*':
            self.Advance()
            p2 = self.ParsePrim()
            p = TBin(p, op, p2)
            op = self.x
        return p
        
    def ParseSum(self):
        p = self.ParseProduct()
        op = self.x
        while op=='+' or op=='-':
            self.Advance()
            p2 = self.ParseProduct()
            p = TBin(p, op, p2)
            op = self.x
        return p

    def ParseRelop(self):
        p = self.ParseSum()
        op = self.x
        while op=='==' or op=='!=' or op=='<' or op=='>' or op=='<=' or op=='>=':
            self.Advance()
            p2 = self.ParseSum()
            p = TBin(p, op, p2)
            op = self.x
        return p

    def ParseAssign(self):
        p  = self.ParseRelop()
        op = self.x
        while op == '=':
            self.Advance()
            if type(p) is not TIdent:
                print>>E, 'type(p) = %s' % type(p)
                raise Exception('bad lhs %s' % p)
            p2 = self.ParseRelop()
            p = TAssign(p, op, p2)
            op = self.x
        return p

    def ParseBlock(self):
        vec = []
        while self.t != L_EOF:
            print>>E, 'nando block', self.t, self.x
            if self.t == L_EOL:
                self.Advance()
                continue
            if self.x == 'print':
                self.Advance()
                a = self.ParseRelop()
                p = TPrint(a)
            elif self.x == 'assert':
                self.Advance()
                a = self.ParseRelop()
                p = TAssert(a)
            else:
                p = self.ParseAssign()
            vec.append(p)
        return TBlock(vec)

class TInt(object):
    def __init__(self, x):
        self.x = int(x)
    def __str__(self):
        return 'TInt(%d)' % self.x
    def visit(self, a):
        return a.visitInt(self);

class TStr(object):
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return 'TStr(%s)' % self.x
    def visit(self, a):
        return a.visitStr(self);

class TIdent(object):
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return 'TIdent(%s)' % self.x
    def visit(self, a):
        return a.visitIdent(self);

class TBin(object):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y
    def __str__(self):
        return 'TBin(%s %s %s)' % (self.x, self.op, self.y)
    def visit(self, a):
        return a.visitBin(self);

class TAssign(object):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y
    def __str__(self):
        return 'TAssign(%s %s %s)' % (self.x, self.op, self.y)
    def visit(self, a):
        return a.visitAssign(self);

class TPrint(object):
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return 'TPrint(%s)' % self.x
    def visit(self, a):
        return a.visitPrint(self);

class TAssert(object):
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return 'TAssert(%s)' % self.x
    def visit(self, a):
        return a.visitAssert(self);

class TBlock(object):
    def __init__(self, vec):
        self.vec = vec
    def __str__(self):
        return 'TBlock(%s)' % [str(x) for x in self.vec]
    def visit(self, a):
        return a.visitBlock(self);

class Compiler(object):
    def __init__(self):
        self.bc = []
        self.interns = {}
        self.globals = {}

    def AddIntern(self, s):
      # self.interns :: s -> (i, patches)
      if s in self.interns:
        i, patches = self.interns[s]
        return i
      z = len(self.interns)
      self.interns[s] = (z, [])
      return z

    def PatchIntern(self, s, patch):
      i, patches = self.interns[s]
      patches.append(patch)
      return i

    def AddGlobal(self, name):
      """Add with intern number."""
      # self.globals :: name -> (j, patches)
      self.AddIntern(name)  # Will need it at Output time.
      if name in self.globals:
        j, patches = self.globals[name]
        return j
      z = len(self.globals)
      self.globals[name] = (z, [])
      return z

    def PatchGlobal(self, name, patch):
      j, patches = self.globals[name]
      patches.append(patch)
      return j

    def visitBlock(self, t):
        print >>E, '//block{{{'
        for e in t.vec:
            e.visit(self)
            print >>E
        print >>E, '//block}}}'

    def visitPrint(self, t):
        t.x.visit(self)
        print >>E, 'BC_Print,'
        self.bc.append('Print')

    def visitAssert(self, t):
        t.x.visit(self)
        print >>E, 'BC_Assert,'
        self.bc.append('Assert')

    def visitAssign(self, t):
        t.y.visit(self)
        if type(t.x) is TIdent:
            # assume GlobalDict for now.
            var = t.x.x
            self.AddGlobal(var)
            self.bc.append('GlobalPut')
            g = self.PatchGlobal(var, len(self.bc))
            self.bc.append(g)
        else:
            raise Exception('visitAssign: bad lhs: %s' % t.x)

    def visitBin(self, t):
        t.x.visit(self)
        t.y.visit(self)
        if t.op == '+':
            print >>E, 'BC_Plus,'
            self.bc.append('Plus')
        elif t.op == '-':
            print >>E, 'BC_Minus,'
            self.bc.append('Minus')
        elif t.op == '*':
            print >>E, 'BC_Times,'
            self.bc.append('Times')
        elif t.op == '==':
            print >>E, 'BC_EQ,'
            self.bc.append('EQ')
        elif t.op == '!=':
            print >>E, 'BC_NE,'
            self.bc.append('NE')
        elif t.op == '<':
            print >>E, 'BC_LT,'
            self.bc.append('LT')
        elif t.op == '>':
            print >>E, 'BC_GT,'
            self.bc.append('GT')
        elif t.op == '<=':
            print >>E, 'BC_LE,'
            self.bc.append('LE')
        elif t.op == '>=':
            print >>E, 'BC_GE,'
            self.bc.append('GE')
        else:
            raise Exception('visitBin: bad %s' % t.op)

    def visitIdent(self, t):
        if type(t.x) is str:
            # assume GlobalDict for now.
            var = t.x
            self.AddGlobal(var)
            self.bc.append('GlobalGet')
            g = self.PatchGlobal(var, len(self.bc))
            self.bc.append(g)
        else:
            raise Exception('visitIdent: bad var: %s %s' % (type(t.x), t.x))
        
    def visitStr(self, t):
            s = t.x
            print >>E, 'BC_LitStr,', str(len(s)), ','
            self.bc.append('LitStr')
            self.bc.append(len(s))
            for ch in s:
                print >>E, "'%s'," % ch
                self.bc.append(ch)
        
    def visitInt(self, t):
        if 0 <= t.x and t.x <= 255:
            print >>E, 'BC_LitInt,', t.x,','
            self.bc.append('LitInt')
            self.bc.append(255 & t.x)
        else:
            raise Exception('visitInt: bad %s' % t.x)

    def Items2Bytecodes(self, bc):
      z = []
      print >>E, 'AAAAA', repr(bc)
      for x in bc:
        print >>E, 'BBBBB', repr(x)
        if type(x) is int:
          z.append (chr(255 & x))
        elif type(x) is str and len(x) == 1:
          z.append (x)
        elif type(x) is str:
          z.append (chr(BytecodeNumbers[x]))
        else:
          raise Exception('bad item: %s %s' % (type(x), x))
      return z

    def OutputCodePack(self, w):
      self.bc.append('Stop')
      P.put_str(w, T.CodePack_bytecode, self.Items2Bytecodes(self.bc))

      # sort by interns by number i, and write to protobuf in that order.
      i_vec = []
      for s, (i, patches) in self.interns.items():
        i_vec.append((i, s, patches))
      for i, s, patches in sorted(i_vec):  # Sorted by i
        P.put_start_message(w, T.CodePack_interns)
        P.put_str(w, T.InternPack_s, s)
        for e in patches:
          P.put_int(w, T.InternPack_patch, e)
        P.put_finish_message(w)

      # sort by globals by number j, and write to protobuf in that order.
      g_vec = []
      for name, (j, patches) in self.globals.items():
        g_vec.append((j, name, patches))
      for j, name, patches in sorted(g_vec):
        P.put_start_message(w, T.CodePack_globals)
        P.put_int(w, T.GlobalPack_name_i, self.AddIntern(name))
        for e in patches:
          P.put_int(w, T.GlobalPack_patch, e)
        P.put_finish_message(w)

      # later: funcpacks
      # later: classpacks

      P.put_finish_message(w)

class AppendWriter:
  def __init__(self, w):
    self.w = w
  def append(self, x):
    self.w.write(chr(x))

if __name__ == '__main__': # test
    XXXXXXXXXXXXXXXXXXXXXXXXXDEMO = '''
x = 100
y = 5 + x
print x + y
'''
    p = Parser(sys.stdin.read())
    block = p.ParseBlock()
    print>>E, block
    for i in range(len(block.vec)):
        print>>E, '%d: %s' % (i, block.vec[i])
    compiler = Compiler()
    compiler.visitBlock(block)
    print >>E, repr(compiler.bc)
    GetBytecodeNumbers()
    print >>E, BytecodeNumbers

    if True:
      compiler.OutputCodePack(AppendWriter(sys.stdout))
    else:
      print '\001\001\001\001',
      for x in compiler.bc:
        if type(x) is int:
          sys.stdout.write(chr(255 & x))
        elif type(x) is str and len(x) == 1:
          sys.stdout.write(x)
        elif type(x) is str:
          sys.stdout.write(chr(BytecodeNumbers[x]))
      print '\001\001\001\001\000'
