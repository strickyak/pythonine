# lex.py -- lexical tokenizer.

import re, sys
import _generated_proto as T  # Tags.
import py_pb as P  # Protocol buffers.
E = sys.stderr

L_EOF = 0
L_INT = 1
L_STR = 2
L_IDENTIFIER = 3
L_MULTI = 4
L_PUNC = 5
L_BOL = 6
P_INDENT = 7
P_DEDENT = 8
P_EOL = 9

BytecodeNumbers = {}


def LexKind(a):
    if a == L_EOF: return 'L_EOF'
    elif a == L_INT: return 'L_INT'
    elif a == L_STR: return 'L_STR'
    elif a == L_IDENTIFIER: return 'L_IDENTIFIER'
    elif a == L_MULTI: return 'L_MULTI'
    elif a == L_PUNC: return 'L_PUNC'
    elif a == L_BOL: return 'L_BOL'
    elif a == P_INDENT: return 'P_INDENT'
    elif a == P_DEDENT: return 'P_DEDENT'
    elif a == P_EOL: return 'P_EOL'
    else: return 'L_???'


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
    return 'A' <= c <= 'Z' or 'a' <= c <= 'z' or c == '_'


def IsWhite(c):
    return (c) and (c <= ' ')


def ShowLex(kind, what):
    print >> E, 'ShowLex => [%s %s]' % (LexKind(kind), repr(what))
    return (kind, what)


class Lexer(object):
    def __init__(self, program):
        self.program = program
        self.i = 0

    def UnGetC(self):
        print >> E, 'UnGetC <-----'
        self.i -= 1

    def GetC(self):
        if self.i >= len(self.program):
            print >> E, 'GetC ----> None'
            return None
        z = self.program[self.i]
        self.i += 1
        print >> E, 'GetC ----> %s' % repr(z)
        return z

    def Next(self):
        c = self.GetC()
        if not c: return ShowLex(L_EOF, None)
        col, eol = 0, False
        while c == '#' or IsWhite(c):
            if c == '#':
                while c != '\n' and c != '\r':
                    c = self.GetC()
                    if not c: return ShowLex(L_EOF, None)
                col, eol = 0, True
            elif c == '\n' or c == '\r':
                col, eol = 0, True
            else:
                if c == '\t':
                    col = ((col + 4) >> 2) << 2
                else:
                    col += 1
            c = self.GetC()

        if not c: return ShowLex(L_EOF, None)
        if eol:
            self.UnGetC()
            return ShowLex(L_BOL, col)

        if IsDigit(c):
            x = 0
            while IsDigit(c):
                x = x * 10 + ord(c) - 48
                c = self.GetC()
            self.UnGetC()
            return ShowLex(L_INT, x)
        if IsAlfa(c):
            x = ''
            while IsAlfa(c) or IsDigit(c):
                x += c
                c = self.GetC()
            self.UnGetC()
            return ShowLex(L_IDENTIFIER, x)
        if c in ['=', '!', '<', '>']:
            d = self.GetC()
            if d in ['=', '<', '>']:
                return ShowLex(L_MULTI, c + d)
            else:
                self.UnGetC()
                # and fallthrough
        return ShowLex(L_PUNC, c)


class Parser(object):
    def __init__(self, program):
        self.program = program
        self.lex = Lexer(program)
        self.indents = [0]
        self.pending_indent = False
        self.pending_dedents = 0
        self.Advance()

    def Advance(self):
        if self.pending_indent:
            self.pending_indent = False
            self.t, self.x = P_INDENT, None
            print >> E, 'ADVANCE -=-=-=-=->', ShowLex(self.t, self.x)
            return
        if self.pending_dedents:
            self.pending_dedents -= 1
            self.t, self.x = P_DEDENT, None
            print >> E, 'ADVANCE -=-=-=-=->', ShowLex(self.t, self.x)
            return
        self.t, self.x = self.lex.Next()
        if self.t == L_BOL:
            if self.x > self.indents[-1]:
                self.indents.append(self.x)
                self.pending_indent = True
                self.t, self.x = P_EOL, None
                print >> E, 'ADVANCE -=-=-=-=->', ShowLex(self.t, self.x)
                return
            if self.x < self.indents[-1]:
                self.indents.pop()
                if self.x not in self.indents:
                    raise Exception('bad DEDENT: %d %s' % (self.x,
                                                           self.indents))
                self.pending_dedents = 1
                while self.indents[-1] != self.x:
                    self.pending_dedents += 1
                    self.indents.pop()
                self.t, self.x = P_EOL, None
                print >> E, 'ADVANCE -=-=-=-=->', ShowLex(self.t, self.x)
                return
            # So self.x == self.indents[-1]
            self.t, self.x = P_EOL, None
        print >> E, 'ADVANCE -=-=-=-=->', ShowLex(self.t, self.x)

    def ParsePrim(self):
        val = self.x
        if self.t == L_INT:
            self.Advance()
            return TInt(val)
        if self.t == L_STR:
            self.Advance()
            return TStr(val)
        if self.t == L_IDENTIFIER:
            self.Advance()
            return TIdent(val)
        raise Exception('bad prim: %s %s' % (self.t, val))

    def ParseProduct(self):
        p = self.ParsePrim()
        op = self.x
        while op == '*':
            self.Advance()
            p2 = self.ParsePrim()
            p = TBin(p, op, p2)
            op = self.x
        return p

    def ParseSum(self):
        p = self.ParseProduct()
        op = self.x
        while op == '+' or op == '-':
            self.Advance()
            p2 = self.ParseProduct()
            p = TBin(p, op, p2)
            op = self.x
        return p

    def ParseRelop(self):
        p = self.ParseSum()
        op = self.x
        while op == '==' or op == '!=' or op == '<' or op == '>' or op == '<=' or op == '>=':
            self.Advance()
            p2 = self.ParseSum()
            p = TBin(p, op, p2)
            op = self.x
        return p

    def ParseNotOp(self):
        return self.ParseRelop()

    def ParseAndOp(self):
        return self.ParseNotOp()

    def ParseOrOp(self):
        return self.ParseAndOp()

    def ParseSingle(self):
        return self.ParseOrOp()

    def ParseWhile(self):
        cond = self.ParseSingle()
        block = self.ColonBlock()
        return TWhile(cond, block)

    def ParseIf(self):
        print >> E, 'Start If', '#', self.indents
        plist = [self.ParseSingle()]
        print >> E, 'first @@@@@@ p_list====', plist
        blist = [self.ColonBlock()]
        print >> E, 'first @@@@@@ b_list====', blist
        print >> E, 'first @@@@@@@@@@@@ TOKEN =', LexKind(
            self.t), self.x, '#', self.indents

        while self.t == P_EOL:
            self.Advance()
        while self.x == 'elif':
            self.Advance()
            plist.append(self.ParseSingle())
            print >> E, 'middle @@@@@@ p_list====', plist
            blist.append(self.ColonBlock())
            print >> E, 'middle @@@@@@ b_list====', blist
            print >> E, 'middle @@@@@@@@@@@@ TOKEN =', LexKind(
                self.t), self.x, '#', self.indents
        belse = None
        print >> E, 'last @@@@@@@@@@@@ TOKEN =', LexKind(
            self.t), self.x, '#', self.indents

        while self.t == P_EOL:
            self.Advance()
        if self.x == 'else':
            self.Advance()
            belse = self.ColonBlock()
        print >> E, 'last @@@@@@ b_else====', belse
        print >> E, 'final @@@@@@@@@@@@ TOKEN =', LexKind(
            self.t), self.x, '#', self.indents

        while self.t == P_EOL:
            self.Advance()
        z = TIf(plist, blist, belse)
        print >> E, 'Finish If', '#', self.indents, '##', repr(z)
        return z

    def ParseAssign(self):
        p = self.ParseSingle()
        op = self.x
        while op == '=':
            self.Advance()
            if type(p) is not TIdent:
                print >> E, 'type(p) = %s' % type(p)
                raise Exception('bad lhs %s' % p)
            p2 = self.ParseSingle()
            p = TAssign(p, op, p2)
            op = self.x
        return p

    def ColonBlock(self):
        if self.x != ':':
            raise Exception('missing colon')
        self.Advance()
        if self.t != P_EOL:
            raise Exception('missing EOL after colon')
        self.Advance()
        if self.t != P_INDENT:
            raise Exception('missing indent after colon')
        self.Advance()
        z = self.ParseBlock()
        if self.t != P_DEDENT and self.t != L_EOF:
            raise Exception('missing newline and dedent after block')
        self.Advance()
        return z

    def ParseBlock(self):
        print >> E, 'Start Block: (((((', '#', self.indents
        vec = []
        while self.t != P_DEDENT and self.t != L_EOF:
            print >> E, 'Think Block: [%s %s]' % (LexKind(self.t),
                                                  self.x), '#', self.indents
            if self.x == 'print':
                self.Advance()
                a = self.ParseRelop()
                p = TPrint(a)
            elif self.x == 'assert':
                self.Advance()
                a = self.ParseRelop()
                p = TAssert(a)
            elif self.x == 'if':
                self.Advance()
                p = self.ParseIf()
            elif self.x == 'while':
                self.Advance()
                p = self.ParseWhile()
            elif self.x == 'pass':
                self.Advance()
                p = None
            else:
                p = self.ParseAssign()
            if p:
                vec.append(p)

            while self.t == P_EOL:
                self.Advance()

        print >> E, 'Finish Block: )))))', '#', self.indents, '>===>', repr(
            vec)
        return TBlock(vec)


class TBase(object):
    def __str__(self):
        return '<%s{%d}>' % (type(self), vars(self))

    def __repr__(self):
        return self.__str__()


class TInt(TBase):
    def __init__(self, x):
        self.x = int(x)

    def __str__(self):
        return 'TInt(%d)' % self.x

    def visit(self, a):
        return a.visitInt(self)


class TStr(TBase):
    def __init__(self, x):
        self.x = x

    def __str__(self):
        return 'TStr(%s)' % self.x

    def visit(self, a):
        return a.visitStr(self)


class TIdent(TBase):
    def __init__(self, x):
        self.x = x

    def __str__(self):
        return 'TIdent(%s)' % self.x

    def visit(self, a):
        return a.visitIdent(self)


class TBin(TBase):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def __str__(self):
        return 'TBin(%s %s %s)' % (self.x, self.op, self.y)

    def visit(self, a):
        return a.visitBin(self)


class TAssign(TBase):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def __str__(self):
        return 'TAssign(%s %s %s)' % (self.x, self.op, self.y)

    def visit(self, a):
        return a.visitAssign(self)


class TWhile(TBase):
    def __init__(self, cond, block):
        self.cond = cond
        self.block = block

    def __str__(self):
        return 'TWhile(%s)' % vars(self)

    def visit(self, a):
        return a.visitWhile(self)


class TIf(TBase):
    def __init__(self, plist, blist, belse):
        self.plist = plist
        self.blist = blist
        self.belse = belse

    def __str__(self):
        return 'TIf(%s)' % vars(self)

    def visit(self, a):
        return a.visitIf(self)


class TPrint(TBase):
    def __init__(self, x):
        self.x = x

    def __str__(self):
        return 'TPrint(%s)' % self.x

    def visit(self, a):
        return a.visitPrint(self)


class TAssert(TBase):
    def __init__(self, x):
        self.x = x

    def __str__(self):
        return 'TAssert(%s)' % self.x

    def visit(self, a):
        return a.visitAssert(self)


class TBlock(TBase):
    def __init__(self, vec):
        self.vec = vec

    def __str__(self):
        return 'TBlock(%s)' % [str(x) for x in self.vec]

    def visit(self, a):
        return a.visitBlock(self)


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
        print >> E, '//block{{{'
        for e in t.vec:
            e.visit(self)
            print >> E
        print >> E, '//block}}}'

    def visitPrint(self, t):
        t.x.visit(self)
        print >> E, 'BC_Print,'
        self.bc.append('Print')

    def visitAssert(self, t):
        t.x.visit(self)
        print >> E, 'BC_Assert,'
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

    def visitWhile(self, t):
        start = len(self.bc)
        t.cond.visit(self)

        self.bc.append('BranchIfFalse')
        patch = len(self.bc)
        self.bc.append(0)  # to the end.

        t.block.visit(self)

        self.bc.append('Branch')
        self.bc.append(start)

        self.bc[patch] = len(self.bc)  # to the end.

    def visitIf(self, t):
        endmarks = []
        for p, b in zip(t.plist, t.blist):
            p.visit(self)
            self.bc.append('BranchIfFalse')
            skipmark = len(self.bc)
            self.bc.append(0)
            b.visit(self)
            self.bc.append('Branch')
            endmarks.append(len(self.bc))
            self.bc.append(0)
            self.bc[skipmark] = len(self.bc)
        if t.belse:
            t.belse.visit(self)
        end = len(self.bc)
        for m in endmarks:
            self.bc[m] = end

    def visitBin(self, t):
        t.x.visit(self)
        t.y.visit(self)
        if t.op == '+':
            print >> E, 'BC_Plus,'
            self.bc.append('Plus')
        elif t.op == '-':
            print >> E, 'BC_Minus,'
            self.bc.append('Minus')
        elif t.op == '*':
            print >> E, 'BC_Times,'
            self.bc.append('Times')
        elif t.op == '==':
            print >> E, 'BC_EQ,'
            self.bc.append('EQ')
        elif t.op == '!=':
            print >> E, 'BC_NE,'
            self.bc.append('NE')
        elif t.op == '<':
            print >> E, 'BC_LT,'
            self.bc.append('LT')
        elif t.op == '>':
            print >> E, 'BC_GT,'
            self.bc.append('GT')
        elif t.op == '<=':
            print >> E, 'BC_LE,'
            self.bc.append('LE')
        elif t.op == '>=':
            print >> E, 'BC_GE,'
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
        print >> E, 'BC_LitStr,', str(len(s)), ','
        self.bc.append('LitStr')
        self.bc.append(len(s))
        for ch in s:
            print >> E, "'%s'," % ch
            self.bc.append(ch)

    def visitInt(self, t):
        if 0 <= t.x and t.x <= 255:
            print >> E, 'BC_LitInt,', t.x, ','
            self.bc.append('LitInt')
            self.bc.append(255 & t.x)
        else:
            raise Exception('visitInt: bad %s' % t.x)

    def Items2Bytecodes(self, bc):
        z = []
        print >> E, 'AAAAA', repr(bc)
        for x in bc:
            print >> E, 'BBBBB', repr(x)
            if type(x) is int:
                z.append(chr(255 & x))
            elif type(x) is str and len(x) == 1:
                z.append(x)
            elif type(x) is str:
                z.append(chr(BytecodeNumbers[x]))
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


if __name__ == '__main__':  # test
    XXXXXXXXXXXXXXXXXXXXXXXXXDEMO = '''
x = 100
y = 5 + x
print x + y
'''
    p = Parser(sys.stdin.read())
    block = p.ParseBlock()
    print >> E, block
    for i in range(len(block.vec)):
        print >> E, '%d: %s' % (i, block.vec[i])
    compiler = Compiler()
    compiler.visitBlock(block)
    print >> E, repr(compiler.bc)
    GetBytecodeNumbers()
    print >> E, BytecodeNumbers

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
