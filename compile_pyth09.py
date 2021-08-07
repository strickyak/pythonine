# lex.py -- lexical tokenizer.

import re, sys
import _generated_proto as T  # Tags.
import py_pb as P  # Protocol buffers.
E = sys.stderr

BC_NUM_ARGS = 0
BC_NUM_LOCALS = 1
BC_NUM_TEMPS = 2
BC_MODULE = 3
BC_CLASS = 4
BC_NAME = 5
BC_HEADER_SIZE = 6

# Created by Lexer:
L_EOF = 0
L_INT = 1
L_STR = 2
L_IDENTIFIER = 3
L_MULTI = 4
L_PUNC = 5
L_BOL = 6
# Created by Parser::Advance:
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
    with open('_generated_prim.h') as fd:
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
    return (kind, what)


class Lexer(object):
    def __init__(self, program):
        self.program = program
        self.i = 0

    def UnGetC(self):
        self.i -= 1

    def GetC(self):
        if self.i >= len(self.program):
            return None
        z = self.program[self.i]
        self.i += 1
        return z

    def Next(self):
        """Next only returns L_BOL (with the indent #) and L_BOL 
           as framing tokens.  Parser::Advance changes L_BOL to
           P_EOL and P_INDENT and P_DEDENT tokens."""
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

        if c == '"':
            s = ''
            c = self.GetC()  # after the initial "
            while c != '"':
                s += c
                c = self.GetC()
            return ShowLex(L_STR, s)

        if c == "'":
            s = ''
            c = self.GetC()  # after the initial '
            while c != "'":
                s += c
                c = self.GetC()
            return ShowLex(L_STR, s)

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
        self.Advance9()
        print >>E, 'Advance', '::', self.t, '::', repr(self.x), '::', repr(self.lex.program[:self.lex.i])

    def Advance9(self):
        """Lexer::Next only returns L_BOL (with the indent column) and L_BOL 
           as framing tokens.  Advance changes L_BOL to P_EOL
           P_INDENT and P_DEDENT tokens."""
        if self.pending_indent:
            self.pending_indent = False
            self.t, self.x = P_INDENT, None
            return
        if self.pending_dedents:
            self.pending_dedents -= 1
            self.t, self.x = P_DEDENT, None
            return
        self.t, self.x = self.lex.Next()
        if self.t == L_BOL:
            if self.x > self.indents[-1]:
                self.indents.append(self.x)
                self.pending_indent = True
                self.t, self.x = P_EOL, None
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
                return
            # So self.x == self.indents[-1]
            self.t, self.x = P_EOL, None

    def ParsePrim(self):
        val = self.x
        if self.t == L_INT:
            self.Advance()
            return TInt(val)
        if self.t == L_STR:
            self.Advance()
            return TStr(val)
        if self.t == L_IDENTIFIER:
            return self.ParseIdentifier()
        if self.x == '[':  # ']'
            return self.ParseList()
        if self.x == '{':  # '}'
            return self.ParseDict()
        if self.x == '(':  # ')'
            return self.ParseParen()
        raise Exception('bad prim: %s %s' % (self.t, val))

    def ParseDict(self):
        dic = []
        self.ConsumeX('{')
        while self.x != '}':
            k = self.ParseSingle()
            self.ConsumeX(':')
            v = self.ParseSingle()
            dic.append((k, v))
            if self.x == ',':
                self.Advance()
            elif self.x != '}':
                raise Exception('expected `,` or `}` after dict item')
        self.Advance()
        return TDict(dic)

    def ParseParen(self):
        vec = []
        self.ConsumeX('(')
        while self.x != ')':
            a = self.ParseSingle()
            vec.append(a)
            if self.x == ',':
                self.Advance()
            elif self.x != ')':
                raise Exception('expected `,` or `)` after paren item')
        self.Advance()
        if len(vec)==1:
            return vec[0]
        else:
            return TTuple(vec)

    def ParseList(self):
        vec = []
        self.ConsumeX('[')
        while self.x != ']':
            a = self.ParseSingle()
            vec.append(a)
            if self.x == ',':
                self.Advance()
            elif self.x != ']':
                raise Exception('expected `,` or `]` after list item')
        self.Advance()
        return TList(vec)

    def ParsePrimEtc(self):
        a = self.ParsePrim()
        while True:
            if self.x == '(':  # FunCall
                self.Advance()
                xlist = self.ParseXList()
                self.ConsumeX(')')
                a = TFunCall(a, xlist)
            elif self.x == '[':  # GetItem
                self.Advance()
                key = self.ParseSingle()
                self.ConsumeX(']')
                a = TGetItem(a, key)
            elif self.x == '.':  # Member
                self.Advance()
                if self.t != L_IDENTIFIER:
                    raise Exception(
                        'expected identifier after `.`, got `%s`' % self.x)
                a = TMember(a, self.x)
                self.Advance()
            else:
                break
        return a

    def ParseIdentifier(self):
        return TIdent(self.ConsumeT(L_IDENTIFIER))

    def ParseProduct(self):
        p = self.ParsePrimEtc()
        op = self.x
        while op == '*':
            self.Advance()
            p2 = self.ParsePrimEtc()
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

    def ParseCommaList(self):
        vec = []
        while True:
            vec.append(self.ParseSingle())
            if self.x != ',':
                break
            self.Advance()
        if len(vec) < 1:
            raise Exception('how < 1')
        elif len(vec) == 1:
            return vec[0]
        else:
            return TTuple(vec)

    def ParseXList(self):
        vec = []
        while self.x != ')':
            vec.append(self.ParseSingle())
            assert self.x == ',' or self.x == ')'
            if self.x == ',':
                self.Advance()
        return vec

    def ParseClass(self):
        name = self.ParseIdentifier().x
        if self.x == '(':
            self.ConsumeX('(')
            if self.x == 'object':
                self.ConsumeX('object')
            self.ConsumeX(')')
        block = self.ColonBlock()
        return TClass(name, block)

    def ParseDef(self):
        name = self.ParseIdentifier().x
        self.ConsumeX('(')
        arglist = self.ParseVarList()
        self.ConsumeX(')')
        block = self.ColonBlock()
        return TDef(name, arglist, block)

    def ParseVarList(self):
        vlist = []
        while self.t == L_IDENTIFIER:
            vlist.append(self.ParseIdentifier().x)
            if self.x == ',':
                self.Advance()
        return vlist

    def ParseRaise(self):
        ex = self.ParseSingle()
        return TRaise(ex)

    def ParseReturn(self):
        if self.t == P_EOL:
            return TReturn(None)
        else:
            retval = self.ParseSingle()
            return TReturn(retval)

    def ParseWhile(self):
        cond = self.ParseSingle()
        block = self.ColonBlock()
        return TWhile(cond, block)

    def ParseTry(self):
        try_block = self.ColonBlock()
        self.ConsumeX('except')
        if self.x == 'as':
            self.ConsumeX('as')
        except_var = None
        if self.t == L_IDENTIFIER:
            except_var = self.ParseIdentifier()
        catch_block = self.ColonBlock()
        return TTry(try_block, except_var, catch_block)

    def ParseIf(self):
        plist = [self.ParseSingle()]
        blist = [self.ColonBlock()]

        while self.t == P_EOL:
            self.Advance()
        while self.x == 'elif':
            self.Advance()
            plist.append(self.ParseSingle())
            blist.append(self.ColonBlock())
        belse = None

        while self.t == P_EOL:
            self.Advance()
        if self.x == 'else':
            self.Advance()
            belse = self.ColonBlock()

        while self.t == P_EOL:
            self.Advance()
        z = TIf(plist, blist, belse)
        return z

    def ParseAssign(self):
        p = self.ParseCommaList()
        op = self.x
        if op == '=':
            while op == '=':
                self.Advance()
                p2 = self.ParseCommaList()
                if type(p) is TIdent or type(p) is TMember or type(p) is TTuple: # TODO nested
                    p = TAssign(p, op, p2)
                    op = self.x
                else:
                    raise Exception('bad lhs %s' % p)
        elif self.t == P_EOL or self.t == L_EOF:
            p = TJustExpr(p)
        else:
            raise Exception('expected = or EOL, but got %s' % self.x)
        return p

    def ConsumeX(self, x):
        if self.x != x:
            raise Exception('expected %s but got %s' % (x, self.x))
        self.Advance()

    def ConsumeT(self, t):
        if self.t != t:
            raise Exception('expected type %s but got %s %s' % (t, self.t,
                                                                self.x))
        z = self.x
        self.Advance()
        return z

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
        vec = []
        while self.t != P_DEDENT and self.t != L_EOF:
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
            elif self.x == 'try':
                self.Advance()
                p = self.ParseTry()
            elif self.x == 'while':
                self.Advance()
                p = self.ParseWhile()
            elif self.x == 'raise':
                self.Advance()
                p = self.ParseRaise()
            elif self.x == 'return':
                self.Advance()
                p = self.ParseReturn()
            elif self.x == 'def':
                self.Advance()
                p = self.ParseDef()
            elif self.x == 'class':
                self.Advance()
                p = self.ParseClass()
            elif self.x == 'pass':
                self.Advance()
                p = None
            else:
                p = self.ParseAssign()
            if p:
                vec.append(p)

            while self.t == P_EOL:
                self.Advance()

        return TBlock(vec)


class TBase(object):
    def __str__(self):
        return '<%s{%s}>' % (type(self), repr(vars(self)))

    def __repr__(self):
        return self.__str__()


class TInt(TBase):
    def __init__(self, x):
        self.x = int(x)

    def visit(self, a):
        return a.visitInt(self)


class TStr(TBase):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitStr(self)


class TDict(TBase):
    def __init__(self, dic):
        self.dic = dic

    def visit(self, a):
        return a.visitDict(self)


class TTuple(TBase):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitTuple(self)


class TList(TBase):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitList(self)


class TGetItem(TBase):
    def __init__(self, coll, key):
        self.coll = coll
        self.key = key

    def visit(self, a):
        return a.visitGetItem(self)


class TIdent(TBase):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitIdent(self)


class TMember(TBase):
    def __init__(self, x, member):
        self.x = x
        self.member = member

    def visit(self, a):
        return a.visitMember(self)


class TBin(TBase):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def visit(self, a):
        return a.visitBin(self)


class TJustExpr(TBase):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitJustExpr(self)


class TAssign(TBase):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def visit(self, a):
        return a.visitAssign(self)


class TFunCall(TBase):
    def __init__(self, fn, xlist):
        self.fn = fn
        self.xlist = xlist

    def visit(self, a):
        return a.visitFunCall(self)


class TDef(TBase):
    def __init__(self, name, arglist, block):
        self.name = name
        self.arglist = arglist
        self.block = block

    def visit(self, a):
        return a.visitDef(self, a.funcs, tclass=None)


class TClass(TBase):
    def __init__(self, name, block):
        self.name = name
        self.block = block
        self.fields = set()  # To be filled in...
        self.funcs = dict()  # To be filled in, as TDefs in block are compiled.

    def visit(self, a):
        return a.visitClass(self)


class TWhile(TBase):
    def __init__(self, cond, block):
        self.cond = cond
        self.block = block

    def visit(self, a):
        return a.visitWhile(self)

class TTry(TBase):
    def __init__(self, try_block, except_var, catch_block):
        self.try_block = try_block
        self.except_var = except_var
        self.catch_block = catch_block

    def visit(self, a):
        return a.visitTry(self)

class TIf(TBase):
    def __init__(self, plist, blist, belse):
        self.plist = plist
        self.blist = blist
        self.belse = belse

    def visit(self, a):
        return a.visitIf(self)


class TRaise(TBase):
    def __init__(self, ex):
        self.ex = ex

    def visit(self, a):
        return a.visitRaise(self)


class TReturn(TBase):
    def __init__(self, retval):
        self.retval = retval

    def visit(self, a):
        return a.visitReturn(self)


class TPrint(TBase):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitPrint(self)


class TAssert(TBase):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitAssert(self)


class TBlock(TBase):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitBlock(self)


class Compiler(object):
    def __init__(self, parentCompiler, argVars, localVars, tclass, isDunderInit):
        self.parentCompiler = parentCompiler
        self.tclass = tclass
        self.isDunderInit = isDunderInit
        argVars = [] if argVars is None else argVars
        localVars = set() if localVars is None else localVars
        self.argVars = argVars
        self.localVars = sorted(localVars - set(argVars))
        print >> E, 'Compiler init:', 'parent', parentCompiler, 'argVars', self.argVars, 'localVars', self.localVars, 'tclass', tclass, 'isDunderInit', isDunderInit

        self.ops = [0, 0, 0, 0xFF, 0xFF, 0xFF]
        self.interns = {}
        self.globals = {}
        self.classes = {}
        self.funcs = {}
        self.tempVars = []  # not used yet

    def AddIntern(self, s):
        # self.interns :: s -> (i, patches)
        if s in self.interns:
            i, patches = self.interns[s]
            return i
        z = len(self.interns)
        self.interns[s] = (z, [])
        return z

    def PatchIntern(self, s, patch):
        self.AddIntern(s)
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
        self.ops.append('Print')

    def visitAssert(self, t):
        t.x.visit(self)
        self.ops.append('Assert')

    def visitJustExpr(self, t):
        t.x.visit(self)
        self.ops.append('Drop')

    def visitAssign(self, t):
        t.y.visit(self)
        self.assignTo(t.x)

    def assignTo(self, a):
        if type(a) is TIdent:
            var = a.x
            if var in self.argVars:
                self.ops.append('ArgPut')
                self.ops.append(self.argVars.index(var))
            elif var in self.localVars:
                self.ops.append('LocalPut')
                self.ops.append(self.localVars.index(var))
            else:
                self.AddGlobal(var)
                self.ops.append('GlobalPut')
                g = self.PatchGlobal(var, len(self.ops))
                self.ops.append(g)
        elif type(a) is TMember:
            if type(a.x) == TIdent and a.x.x == 'self' and self.tclass:
                self.ops.append('SelfMemberPut')
                self.ops.append(sorted(self.tclass.fields).index(a.member))
            else:
                a.x.visit(self)
                self.ops.append('MemberPut')
                isn = self.PatchIntern(a.member, len(self.ops))
                self.ops.append(isn)
        elif type(a) is TTuple:
            self.ops.append('Explode')
            self.ops.append(len(a.vec))
            for b in a.vec:
                self.assignTo(b)
        else:
            raise Exception('assignTo: bad lhs: %s' % a)

    def visitFunCall(self, t):
        # fn, xlist
        for x in t.xlist[::-1]:  # Iterate in reverse.
            x.visit(self)
        if type(t.fn) == TMember:
            t.fn.x.visit(self)
            self.ops.append('CallMeth')
            isn = self.PatchIntern(t.fn.member, len(self.ops))
            self.ops.append(isn)
            self.ops.append(len(t.xlist) + 1)  # +1 for self.
        elif type(t.fn) == TIdent and t.fn.x == 'len' and len(t.xlist) == 1:
            self.ops.append('Len')
        elif type(t.fn) == TIdent and t.fn.x == 'hex' and len(t.xlist) == 1:
            self.ops.append('Hex')
        elif type(t.fn) == TIdent and t.fn.x == 'chr' and len(t.xlist) == 1:
            self.ops.append('Chr')
        elif type(t.fn) == TIdent and t.fn.x == 'ord' and len(t.xlist) == 1:
            self.ops.append('Ord')
        else:
            t.fn.visit(self)
            self.ops.append('Call')
            self.ops.append(len(t.xlist))

    def visitClass(self, t):
        self.AddGlobal(t.name)
        for e in t.block.vec:
            lg = AssignmentVisitor()
            lg.visitBlock(e.block)
            t.fields.update(lg.selfFields)

        for e in t.block.vec:
            if type(e) != TDef:
                raise Exception('not supported in class %s: %s' % (t.name, e))
            self.visitDef(e, t.funcs, t)

        t.fields = sorted(t.fields)

        for f in t.fields:
            self.AddIntern(f)
        self.classes[t.name] = t

    def visitDef(self, t, func_dict, tclass):
        self.AddIntern(t.name)
        if not tclass:
            self.AddGlobal(t.name)
        # First pass, pure functions only, no globals.
        # -- name, varlist==arglist, block.
        lg = AssignmentVisitor()
        lg.localVars.update(t.arglist)  # args are localVars.
        lg.visitBlock(t.block)

        #// def __init__(self, parentCompiler, argVars, localVars, tclass, isDunderInit):
        fc = Compiler(self, t.arglist, lg.localVars, tclass, t.name == '__init__')
        fc.visitBlock(t.block)
        if not len(fc.ops) or (
                fc.ops[-1] != 'Return' and
                fc.ops[-1] != 'RetNone' and
                fc.ops[-1] != 'RetSelf'):
            fc.ops.append('RetSelf' if fc.isDunderInit else 'RetNone')
        func_dict[t.name] = fc

    def visitTry(self, t):
        self.ops.append('Try')
        patch_try = len(self.ops)
        self.ops.append(0)  # to the end.

        t.try_block.visit(self)
        self.ops[patch_try] = len(self.ops)

        self.ops.append('Catch')
        patch_catch = len(self.ops)
        self.ops.append(0)  # to the end.
        if t.except_var:
            self.ops.append(self.localVars.index(t.except_var.x))

        t.catch_block.visit(self)
        self.ops[patch_catch] = len(self.ops)


    def visitWhile(self, t):
        start = len(self.ops)
        t.cond.visit(self)

        self.ops.append('BranchIfFalse')
        patch = len(self.ops)
        self.ops.append(0)  # to the end.

        t.block.visit(self)

        self.ops.append('Branch')
        self.ops.append(start)

        self.ops[patch] = len(self.ops)  # to the end.

    def visitRaise(self, t):
        t.ex.visit(self)
        self.ops.append('Raise')

    def visitReturn(self, t):
        rv = t.retval
        if rv:
            rv.visit(self)
        if self.isDunderInit:
            if rv:
                self.ops.append('Drop')
            self.ops.append('RetSelf')
        elif rv:
            self.ops.append('Return')
        else:
            self.ops.append('RetNone')

    def visitIf(self, t):
        endmarks = []
        for p, b in zip(t.plist, t.blist):
            p.visit(self)
            self.ops.append('BranchIfFalse')
            skipmark = len(self.ops)
            self.ops.append(0)
            b.visit(self)
            self.ops.append('Branch')
            endmarks.append(len(self.ops))
            self.ops.append(0)
            self.ops[skipmark] = len(self.ops)
        if t.belse:
            t.belse.visit(self)
        end = len(self.ops)
        for m in endmarks:
            self.ops[m] = end

    def visitBin(self, t):
        t.x.visit(self)
        t.y.visit(self)
        if t.op == '+':
            self.ops.append('Plus')
        elif t.op == '-':
            self.ops.append('Minus')
        elif t.op == '*':
            self.ops.append('Times')
        elif t.op == '==':
            self.ops.append('EQ')
        elif t.op == '!=':
            self.ops.append('NE')
        elif t.op == '<':
            self.ops.append('LT')
        elif t.op == '>':
            self.ops.append('GT')
        elif t.op == '<=':
            self.ops.append('LE')
        elif t.op == '>=':
            self.ops.append('GE')
        else:
            raise Exception('visitBin: bad %s' % t.op)

    def visitMember(self, t):
        if type(t.x) is TIdent and t.x.x == 'self' and self.tclass:
            self.ops.append('SelfMemberGet')
            self.ops.append(sorted(self.tclass.fields).index(t.member)*2)
        else:
            t.x.visit(self)
            self.ops.append('MemberGet')
            isn = self.PatchIntern(t.member, len(self.ops))
            self.ops.append(isn)

    def visitGetItem(self, t):
        t.coll.visit(self)
        t.key.visit(self)
        self.ops.append('GetItem')

    def visitTuple(self, t):
        for e in t.vec:
            e.visit(self)
        self.ops.append('NewTuple')
        self.ops.append(len(t.vec))

    def visitList(self, t):
        for e in t.vec:
            e.visit(self)
        self.ops.append('NewList')
        self.ops.append(len(t.vec))

    def visitDict(self, t):
        for k, v in t.dic:
            k.visit(self)
            v.visit(self)
        self.ops.append('NewDict')
        self.ops.append(len(t.dic))

    def visitIdent(self, t):
        var = t.x
        if type(var) is str:
            if var in self.argVars:
                self.ops.append('ArgGet')
                self.ops.append(self.argVars.index(var))
            elif var in self.localVars:
                self.ops.append('LocalGet')
                self.ops.append(self.localVars.index(var))
            else:
                self.AddGlobal(var)
                self.ops.append('GlobalGet')
                g = self.PatchGlobal(var, len(self.ops))
                self.ops.append(g)
        else:
            raise Exception('visitIdent: bad var: %s %s' % (type(var), var))

    def visitStr(self, t):
        self.ops.append('LitStr')
        isn = self.PatchIntern(t.x, len(self.ops))
        self.ops.append(isn)

    def visitInt(self, t):
        if 0 <= t.x and t.x <= 255:
            self.ops.append('LitInt')
            self.ops.append(255 & t.x)
        else:
            hi, lo = 255&(t.x>>8), 255&t.x
            self.ops.append('LitInt2')
            self.ops.append(hi)
            self.ops.append(lo)


    def OpList2Bytecodes(self, ops):
        ops[BC_NUM_ARGS] = len(self.argVars)
        ops[BC_NUM_LOCALS] = len(self.localVars)
        ops[BC_NUM_TEMPS] = len(self.tempVars)
        z = []
        for x in ops:
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
        P.put_str(w, T.CodePack_bytecode, self.OpList2Bytecodes(self.ops))

        # sort by interns by number i, and write to protobuf in that order.
        i_vec = []
        for s, (i, patches) in self.interns.items():
            i_vec.append((i, s, patches))
        i_vec = sorted(i_vec)  # Sorted by i
        i_strs = []
        for i, s, patches in i_vec:
            assert i == len(i_strs)
            i_strs.append(s)
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

        # funcpacks
        for name, fc in sorted(self.funcs.items()):
            P.put_start_message(w, T.CodePack_funcpacks)
            P.put_int(w, T.FuncPack_name_i, i_strs.index(name))

            P.put_start_message(w, T.FuncPack_pack)
            fc.OutputCodePack(w)

            P.put_finish_message(w)  # finish CodePack_funcpacks

        # classpacks
        for name, tclass in sorted(self.classes.items()):
            P.put_start_message(w, T.CodePack_classpacks)
            P.put_int(w, T.ClassPack_name_i, i_strs.index(name))

            for fieldName in tclass.fields:
                isn = self.AddIntern(fieldName)
                P.put_int(w, T.ClassPack_field_i, isn)

            for methName, fc in sorted(tclass.funcs.items()):
                P.put_start_message(w, T.ClassPack_meth)

                P.put_int(w, T.FuncPack_name_i, i_strs.index(methName))
                P.put_start_message(w, T.FuncPack_pack)
                fc.OutputCodePack(w)
                P.put_finish_message(w)  # finish ClassPack_meth

            P.put_finish_message(w)  # finish CodePack_classpacks

        P.put_finish_message(w)


class AssignmentVisitor(object):
    def __init__(self):
        self.localVars = set()  # Others are global.
        self.selfFields = set()

    def visitBlock(self, t):
        for e in t.vec:
            e.visit(self)

    def visitPrint(self, t):
        pass

    def visitAssert(self, t):
        pass

    def visitRaise(self, t):
        pass
    def visitReturn(self, t):
        pass

    def visitAssign(self, t):
        if type(t.x) is TIdent:
            self.localVars.add(t.x.x)
        elif type(t.x) is TMember and type(
                t.x.x) is TIdent and t.x.x.x == 'self':
            self.selfFields.add(t.x.member)
        elif type(t.x) is TMember and type(t.x.x) is TIdent:
            pass  # Foreign member.
        else:
            raise Exception('visitAssign: bad lhs: %s' % t.x)

    def visitClass(self, t):
        Abort()  # No nested classes.

    def visitDef(self, t, func_dict, tclass):
        Abort()  # No nested functions, yet.

    def visitTry(self, t):
        t.try_block.visit(self)
        t.catch_block.visit(self)
        if t.except_var:
            self.localVars.add(t.except_var.x)

    def visitWhile(self, t):
        t.block.visit(self)

    def visitIf(self, t):
        for p, b in zip(t.plist, t.blist):
            b.visit(self)
        if t.belse:
            t.belse.visit(self)

    def visitBin(self, t):
        pass

    def visitIdent(self, t):
        if type(t.x) is str:
            # assume GlobalDict for now.
            var = t.x
            self.AddGlobal(var)
            self.ops.append('GlobalGet')
            g = self.PatchGlobal(var, len(self.ops))
            self.ops.append(g)
        else:
            raise Exception('visitIdent: bad var: %s %s' % (type(t.x), t.x))

    def visitStr(self, t):
        pass

    def visitInt(self, t):
        pass


class BytesWriter(object):
    def __init__(self):
        self.v = []

    def append(self, x):
        self.v.append(x)

    def Bytes(self):
        return ''.join([chr(e) for e in self.v])


class AppendWriter(object):
    def __init__(self, w):
        self.w = w

    def append(self, x):
        self.w.write(chr(x))


if __name__ == '__main__':  # test
    GetBytecodeNumbers()
    p = Parser(sys.stdin.read())
    block = p.ParseBlock()
    compiler = Compiler(None, None, None, None, False)
    compiler.visitBlock(block)
    compiler.ops.append('RetNone')

    compiler.OutputCodePack(AppendWriter(sys.stdout))
