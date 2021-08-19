## compile_pyth09.py -- tokenizer, parser, and code generator for Pythonine.

#if UNIX
import re
import sys
Stdin = sys.stdin
Stdout = sys.stdout
Stderr = sys.stderr
def is_in(a, b): return (a in b)
def is_not_in(a, b): return (a not in b)

import _generated_proto as T  # Tags.
import py_pb as P  # Protocol buffers.

#endif

E = Stderr

#if COCO
def is_in(a, b):
    for e in b:
        if e == a: return True
    return False
def is_not_in(a, b):
    return not is_in(a, b)
def reversed(vec):
    z = []
    n = len(vec)
    for i in range(len(vec)):
        z.append(vec[n - i - 1])
    return z

#include "_generated_proto.py"
#include "py_pb.py"

#endif

def Inside(x):
    if type(x) == list or type(x) == set:
        s = '[*  '
        for e in x:
            s = s + Inside(e) + ' , '
        return s + '*]'
    try:
        return str(vars(x))
    except:
        return str(x)

BC_NUM_ARGS = 0
BC_NUM_LOCALS = 1
BC_NUM_TEMPS = 2
BC_MODULE = 3
BC_CLASS = 4
BC_NAME = 5
BC_HEADER_SIZE = 6

## Created by Lexer:
L_EOF = 0
L_INT = 1
L_STR = 2
L_IDENTIFIER = 3
L_MULTI = 4
L_PUNC = 5
L_BOL = 6
## Created by Parser::Advance:
P_INDENT = 7
P_DEDENT = 8
P_EOL = 9

STOPPERS = [']', '}', ')', ';']

#if UNIX

BytecodeNumbers = {}
SerialCounter = [0]

def SerialName():
    n = SerialCounter[0]
    n = n + 1
    SerialCounter[0] = n
    return '__%d' % n

def GetBytecodeNumbers():
    ## Demo: BC_Print = 7,
    regexp = re.compile('BC_([A-Za-z0-9_]+) = ([0-9]+),')
    with open('_generated_prim.h') as fd:
        for line in fd:
            m = regexp.match(line.strip())
            if m:
                BytecodeNumbers[m.group(1)] = int(m.group(2))


#endif

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
    else: return 'L_UNKNOWN'


def IsDigit(c):
    return '0' <= c and c <= '9'


def IsAlfa(c):
    return 'A' <= c and c <= 'Z' or 'a' <= c and c <= 'z' or c == '_'


def IsWhite(c):
    return (c) and (c <= ' ')


def ShowLex(kind, what):
    return (kind, what)


class Lexer(object):
    def __init__(self, program):
        self.program = program
        self.i = 0

    def UnGetC(self):
        self.i = self.i - 1

    def GetC(self):
        if self.i >= len(self.program):
            return None
        z = self.program[self.i]
        self.i = self.i + 1
        return z

    def Next(self):
        ## Next only returns L_BOL (with the indent #) and L_BOL
        ##    as framing tokens.  Parser::Advance changes L_BOL to
        ##    P_EOL and P_INDENT and P_DEDENT tokens.
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
                    col = col + 1
            c = self.GetC()

        return self.Next2(c, col, eol)

    def Next2(self, c, col, eol):
        if not c: return ShowLex(L_EOF, None)
        if eol:
            self.UnGetC()
            return ShowLex(L_BOL, col)

        if c == '"':
            s = ''
            c = self.GetC()  # after the initial '"'
            while c != '"':
                s = s + c
                c = self.GetC()
            return ShowLex(L_STR, s)

        if c == "'":
            s = ''
            c = self.GetC()  # after the initial "'"
            while c != "'":
                s = s + c
                c = self.GetC()
            return ShowLex(L_STR, s)

        return self.Next3(c)

    def Next3(self, c):
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
                x = x + c
                c = self.GetC()
            self.UnGetC()
            return ShowLex(L_IDENTIFIER, x)
        if is_in(c, ['=', '!', '<', '>']):
            d = self.GetC()
            if is_in(d, ['=', '<', '>']):
                return ShowLex(L_MULTI, c + d)
            else:
                self.UnGetC()
                ## and fallthrough
        return ShowLex(L_PUNC, c)


## Python Precedence:
## await x
## x**...
## +x, -x, ~x
## ...**x
## * @ / // %
## + -
## << >>
## &
## ^
## |
## in, not in, is, is not, <, <=, >, >=, !=, ==
## not x
## and
## or
## if ... else
## lambda
## assignment


class Parser(object):
    def __init__(self, program):
        self.program = program
        self.lex = Lexer(program)
        self.indents = [0]
        self.pending_indent = False
        self.pending_dedents = 0
        self.Advance()

    def Advance(self):
        self.Advance_()
        ## print >>E, 'Advance', '::', self.t, '::', repr(self.x), '::', repr(self.lex.program[:self.lex.i])
        print >>E, 'Advance', '::', self.t, '::', repr(self.x)

    def Advance_(self):
        ## Lexer::Next only returns L_BOL (with the indent column) and L_BOL
        ##    as framing tokens.  Advance changes L_BOL to P_EOL
        ##    P_INDENT and P_DEDENT tokens.
        if self.pending_indent:
            self.pending_indent = False
            self.t, self.x = P_INDENT, None
            return
        if self.pending_dedents:
            self.pending_dedents = self.pending_dedents - 1
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
                if is_not_in(self.x, self.indents):
                    raise Exception('bad DEDENT: %d %s' % (self.x, self.indents))
                self.pending_dedents = 1
                while self.indents[-1] != self.x:
                    self.pending_dedents = self.pending_dedents + 1
                    self.indents.pop()
                self.t, self.x = P_EOL, None
                return
            ## So self.x == self.indents[-1]
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
        if self.t == L_PUNC and self.x == '[':  # ']'
            return self.ParseList()
        if self.t == L_PUNC and self.x == '{':  # '}'
            return self.ParseDict()
        if self.t == L_PUNC and self.x == '(':  # ')'
            self.ConsumeX('(')
            x = self.ParseCommaList(False)
            self.ConsumeX(')')
            return x
        raise Exception('bad prim: %s %s' % (self.t, val))

    def ParseDict(self):
        dic = []
        self.ConsumeX('{')
        while self.t != L_PUNC or self.x != '}':
            k = self.ParseSingle()
            self.ConsumeX(':')
            v = self.ParseSingle()
            dic.append((k, v))
            if self.t == L_PUNC and self.x == ',':
                self.Advance()
            elif self.t != L_PUNC or self.x != '}':
                raise Exception('expected `,` or `}` after dict item')
        self.Advance()
        return TDict(dic)

    def ParseList(self):
        vec = []
        self.ConsumeX('[')
        while self.t != L_PUNC or self.x != ']':
            a = self.ParseSingle()
            vec.append(a)
            if self.t == L_PUNC and self.x == ',':
                self.Advance()
            elif self.t != L_PUNC or self.x != ']':
                raise Exception('expected `,` or `]` after list item')
        self.Advance()
        return TList(vec)

    def ParsePrimEtc(self):
        a = self.ParsePrim()
        while True:
            if self.t == L_PUNC and self.x == '(':  # FunCall
                self.Advance()
                xlist = self.ParseCommaList(True).vec
                self.ConsumeX(')')
                a = TFunCall(a, xlist)
            elif self.t == L_PUNC and self.x == '[':  # GetItem
                self.Advance()
                key = self.ParseCommaList(False)
                self.ConsumeX(']')
                a = TGetItem(a, key)
            elif self.t == L_PUNC and self.x == '.':  # Member
                self.Advance()
                if self.t != L_IDENTIFIER:
                    raise Exception('expected identifier after `.`, got `%s`' % self.x)
                a = TMember(a, self.x)
                self.Advance()
            else:
                break
        return a

    def ParseIdentifier(self):
        var = self.ConsumeT(L_IDENTIFIER)
        if var=='in' or var=='if' or var=='is':
            raise Exception('bad var: %s', var)
        if var=='True' or var=='False' or var=='None':
            return TSpecial(var)
        ## if var=='Stdin' or var=='Stdout' or var=='Stderr':
        ##     return TSpecial(var)
        return TIdent(var)

    def ParseUnary(self):
        if self.t == L_PUNC and self.x == '-':
            op = self.x
            self.Advance()
            a = self.ParsePrimEtc()
            return TUnaryOp(a, op)
        return self.ParsePrimEtc()

    def ParseProduct(self):
        p = self.ParseUnary()
        op = self.x
        while op == '*' or op == '%':
            self.Advance()
            p2 = self.ParseUnary()
            p = TBinaryOp(p, op, p2)
            op = self.x
        return p

    def ParseSum(self):
        p = self.ParseProduct()
        op = self.x
        while op == '+' or op == '-':
            self.Advance()
            p2 = self.ParseProduct()
            p = TBinaryOp(p, op, p2)
            op = self.x
        return p

    def ParseMiscop(self):
        p = self.ParseSum()
        op = self.x
        p2 = None
        while op == '<<' or op == '>>' or op == '&' or op == '|' or op == '^':
            if p2:  # do not cascade!
                raise Exception('cascade_misc_op')
            self.Advance()
            p2 = self.ParseSum()
            p = TBinaryOp(p, op, p2)
            op = self.x
        return p

    def ParseRelop(self):
        p = self.ParseMiscop()
        op = self.x
        p2 = None
        while op == '==' or op == '!=' or op == '<' or op == '>' or op == '<=' or op == '>=' or op=='is':
            if p2:  # do not cascade!
                raise Exception('cascade_rel_op')
            self.Advance()
            p2 = self.ParseMiscop()
            p = TBinaryOp(p, op, p2)
            op = self.x
        return p

    def ParseNotOp(self):
        if self.t == L_IDENTIFIER and self.x == 'not':
            self.Advance()
            return TUnaryOp(self.ParseNotOp(), 'not')
        return self.ParseRelop()

    def ParseAndOp(self):
        a = self.ParseNotOp()
        if self.t == L_IDENTIFIER and self.x == 'and':
            vec = [a]
            while self.t == L_IDENTIFIER and self.x == 'and':
                self.Advance()
                vec.append(self.ParseNotOp())
            return TAndOr(vec, 'and')
        else:
            return a

    def ParseOrOp(self):
        a = self.ParseAndOp()
        if self.t == L_IDENTIFIER and self.x == 'or':
            vec = [a]
            while self.t == L_IDENTIFIER and self.x == 'or':
                self.Advance()
                vec.append(self.ParseAndOp())
            return TAndOr(vec, 'or')
        else:
            return a

    def ParseCond(self):
        a = self.ParseOrOp()
        if self.t == L_IDENTIFIER and self.x == 'if':
            self.Advance()
            b = self.ParseOrOp()
            if self.t == L_IDENTIFIER and self.x == 'else':
                self.Advance()
                c = self.ParseCond()
                return TCond(b, a, c)
            raise Exception('expected `else` but got %s', self.x)
        return a

    def ParseSingle(self):
        return self.ParseCond()

    def ParseCommaList(self, force_tuple):
        ## Without the parens. Should be used in lots of places.
        vec = []
        while True:
            if self.t == P_EOL or (self.t == L_PUNC and is_in(self.x, STOPPERS)):
                break
            a = self.ParseSingle()
            vec.append(a)
            if self.t == L_PUNC and self.x == ',':
                force_tuple = True
                self.Advance()
            else:
                break
        if force_tuple or len(vec) != 1:
            return TTuple(vec)
        else:
            return vec[0]

    def ConsumeX(self, x):
        if self.x != x:
            raise Exception('expected %s but got %s' % (x, self.x))
        self.Advance()

    def ConsumeT(self, t):
        if self.t != t:
            raise Exception('expected type %s but got %s %s' % (t, self.t, self.x))
        z = self.x
        self.Advance()
        return z


#if BIG

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
        vec = self.ParseCommaList(True).vec
        arglist = []
        for e in vec:
            assert type(e) == TIdent, 'formal param not identifier: %s' % e
            arglist.append(e.x)
        self.ConsumeX(')')
        block = self.ColonBlock()
        return TDef(name, arglist, block)

    def ParseVarList(self):
        vlist = []
        while True:
            if self.x == '(':
                self.Advance()
                sublist = self.ParseVarList()
                vlist.append(sublist)
                self.ConsumeX(')')
            elif self.x == 'in':
                break
            elif self.t == L_IDENTIFIER:
                var = self.ParseIdentifier().x
                vlist.append(var)
            else:
                break
            if self.x == ',':
                self.Advance()
        return vlist

    def ParseImport(self):
        tup = self.ParseCommaList(True)
        return None # TODO
        ## return TImport(tup.vec)

    def ParseRaise(self):
        ex = self.ParseCommaList(False)
        return TRaise(ex)

    def ParseGlobal(self):
        var = self.ParseIdentifier()
        return TGlobal(var)

    def ParseReturn(self):
        if self.t == P_EOL:
            return TReturn(None)
        else:
            retval = self.ParseCommaList(False)
            return TReturn(retval)

    def ParseBreak(self):
        assert self.t == P_EOL
        return TBreak()
    def ParseContinue(self):
        assert self.t == P_EOL
        return TContinue()
    def ParseWhile(self):
        cond = self.ParseSingle()
        block = self.ColonBlock()
        return TWhile(cond, block)

    def ParseTry(self):
        try_block = self.ColonBlock()
        self.ConsumeX('except')
        if self.x == 'Exception':
            self.Advance()
        if self.x == 'as':
            self.Advance()
            except_var = self.ParseCommaList(False)
        else:
            except_var = TIdent('_')
        catch_block = self.ColonBlock()
        return TTry(try_block, except_var, catch_block)

    def ParseFor(self):
        dest = self.ParseCommaList(False)
        self.ConsumeX('in')
        coll = self.ParseCommaList(False)
        block = self.ColonBlock()
        iterTemp = SerialName()
        return TFor(dest, coll, iterTemp, block)

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
        p = self.ParseCommaList(False)
        op = self.x
        if op == '=':
            while op == '=':  ## this looks broken.
                self.Advance()
                p2 = self.ParseCommaList(False)
                if type(p) is TIdent or type(p) is TMember or type(p) is TGetItem or type(p) is TTuple: # TODO nested
                    p = TAssign(p, op, p2)  ## this looks broken.
                    op = self.x
                else:
                    raise Exception('bad lhs %s' % p)
        elif self.t == P_EOL or self.t == L_EOF:
            p = TExprAndDrop(p)
        else:
            raise Exception('expected = or EOL, but got %s' % self.x)
        return p

    def ColonBlock(self):
        if self.x != ':':
            raise Exception('missing colon')
        self.Advance()

        if self.t != P_EOL:
            ## stmt on same line after colon.
            z = self.ParseStatement()
            while self.t == P_EOL:
                self.Advance()
            ## z can be null, if stmt is `pass`.
            return TBlock([z]) if z else TBlock([])
        self.Advance()  # consume eol

        if self.t != P_INDENT:
            raise Exception('missing indent after colon')
        self.Advance()
        z = self.ParseBlock()
        if self.t != P_DEDENT and self.t != L_EOF:
            raise Exception('missing dedent after block')
        self.Advance()
        return z

    def ParseBlock(self):
        vec = []
        while self.t != P_DEDENT and self.t != L_EOF:
            if self.t == P_EOL:
                self.Advance()
            else:
                stmt = self.ParseStatement()
                if stmt:
                    vec.append(stmt)

        return TBlock(vec)

    def ParseAssert(self):
        print >>E, 'ASSERT:'
        pred = self.ParseSingle()
        print >>E, 'ASSERT: pred:', pred
        msg = None
        if self.x == ',':
            self.Advance()
            msg = self.ParseSingle()
            print >>E, 'ASSERT: msg:', msg
        return TAssert(pred, msg)

    def ParsePrint(self):
        f = None
        if self.t == L_MULTI and self.x=='>>':
            self.Advance()
            f = self.ParseSingle()
            self.ConsumeX(',')
        tup = self.ParseCommaList(True)
        return TPrint(f, tup.vec)

    def ParseStatement(self):
        if self.x == 'print':
            self.Advance()
            p = self.ParsePrint()
        elif self.x == 'assert':
            self.Advance()
            p = self.ParseAssert()
        elif self.x == 'if':
            self.Advance()
            p = self.ParseIf()
        elif self.x == 'try':
            self.Advance()
            p = self.ParseTry()
        elif self.x == 'for':
            self.Advance()
            p = self.ParseFor()
        elif self.x == 'continue':
            self.Advance()
            p = self.ParseContinue()
        elif self.x == 'break':
            self.Advance()
            p = self.ParseBreak()
        elif self.x == 'while':
            self.Advance()
            p = self.ParseWhile()
        elif self.x == 'raise':
            self.Advance()
            p = self.ParseRaise()
        elif self.x == 'return':
            self.Advance()
            p = self.ParseReturn()
        elif self.x == 'global':
            self.Advance()
            p = self.ParseGlobal()
        elif self.x == 'def':
            self.Advance()
            p = self.ParseDef()
        elif self.x == 'class':
            self.Advance()
            p = self.ParseClass()
        elif self.x == 'import':
            self.Advance()
            p = self.ParseImport()
        elif self.x == 'pass':
            self.Advance()
            p = None
        else:
            p = self.ParseAssign()
        return p

#endif

## class TBase(object):
##     def __str__(self):
##         return '<%s{%s}>' % (type(self), repr(vars(self)))
## 
##     def __repr__(self):
##         return self.__str__()


class TSpecial(object):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitSpecial(self)


class TInt(object):
    def __init__(self, x):
        self.x = int(x)

    def visit(self, a):
        return a.visitInt(self)


class TStr(object):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitStr(self)


class TDict(object):
    def __init__(self, dic):
        self.dic = dic

    def visit(self, a):
        return a.visitDict(self)


class TTuple(object):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitTuple(self)


class TList(object):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitList(self)


class TGetItem(object):
    def __init__(self, coll, key):
        self.coll = coll
        self.key = key

    def visit(self, a):
        return a.visitGetItem(self)


class TIdent(object):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitIdent(self)


class TMember(object):
    def __init__(self, x, member):
        self.x = x
        self.member = member

    def visit(self, a):
        return a.visitMember(self)


class TAndOr(object):
    def __init__(self, vec, op):
        self.vec = vec
        self.op = op

    def visit(self, a):
        return a.visitAndOr(self)


class TCond(object):
    def __init__(self, cond, yes, no):
        self.cond = cond
        self.yes = yes
        self.no = no

    def visit(self, a):
        return a.visitCond(self)


class TUnaryOp(object):
    def __init__(self, x, op):
        self.x = x
        self.op = op

    def visit(self, a):
        return a.visitUnaryOp(self)


class TBinaryOp(object):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def visit(self, a):
        return a.visitBinaryOp(self)


class TExprAndDrop(object):
    def __init__(self, x):
        self.x = x

    def visit(self, a):
        return a.visitExprAndDrop(self)


class TAssign(object):
    def __init__(self, x, op, y):
        self.x = x
        self.op = op
        self.y = y

    def visit(self, a):
        return a.visitAssign(self)


class TFunCall(object):
    def __init__(self, fn, xlist):
        self.fn = fn
        self.xlist = xlist

    def visit(self, a):
        return a.visitFunCall(self)

#if BIG

class TDef(object):
    def __init__(self, name, arglist, block):
        self.name = name
        self.arglist = arglist
        self.block = block

    def visit(self, a):
        return a.visitDef(self, a.funcs, tclass=None)


class TClass(object):
    def __init__(self, name, block):
        self.name = name
        self.block = block
        self.fields = set()  # To be filled in...
        self.funcs = dict()  # To be filled in, as TDefs in block are compiled.

    def visit(self, a):
        return a.visitClass(self)


class TBreak(object):
    def __init__(self):
        pass

    def visit(self, a):
        return a.visitBreak(self)

class TContinue(object):
    def __init__(self):
        pass

    def visit(self, a):
        return a.visitContinue(self)

class TWhile(object):
    def __init__(self, cond, block):
        self.cond = cond
        self.block = block

    def visit(self, a):
        return a.visitWhile(self)

class TFor(object):
    def __init__(self, dest, coll, iterTemp, block):
        self.dest = dest
        self.coll = coll
        self.iterTemp = iterTemp
        self.block = block

    def visit(self, a):
        return a.visitFor(self)

class TTry(object):
    def __init__(self, try_block, except_var, catch_block):
        self.try_block = try_block
        self.except_var = except_var
        self.catch_block = catch_block

    def visit(self, a):
        return a.visitTry(self)

class TIf(object):
    def __init__(self, plist, blist, belse):
        self.plist = plist
        self.blist = blist
        self.belse = belse

    def visit(self, a):
        return a.visitIf(self)


## class TImport(object):
##     def __init__(self, vec):
##         self.vec = vec
## 
##     def visit(self, a):
##         return a.visitImport(self)


class TRaise(object):
    def __init__(self, ex):
        self.ex = ex

    def visit(self, a):
        return a.visitRaise(self)


class TGlobal(object):
    def __init__(self, var):
        self.var = var

    def visit(self, a):
        return a.visitGlobal(self)


class TReturn(object):
    def __init__(self, retval):
        self.retval = retval

    def visit(self, a):
        return a.visitReturn(self)


class TPrint(object):
    def __init__(self, f, vec):
        self.f = f
        self.vec = vec

    def visit(self, a):
        return a.visitPrint(self)


class TAssert(object):
    def __init__(self, pred, msg):
        self.pred = pred
        self.msg = msg

    def visit(self, a):
        return a.visitAssert(self)


class TBlock(object):
    def __init__(self, vec):
        self.vec = vec

    def visit(self, a):
        return a.visitBlock(self)

#endif

class Compiler(object):
    def __init__(self, parentCompiler, argVars, localVars, globalOverrides, tclass, isDunderInit):
        self.parentCompiler = parentCompiler
        self.tclass = tclass
        self.isDunderInit = isDunderInit
        ## argVars = [] if argVars is None else argVars
        ## localVars = set() if localVars is None else localVars
        self.argVars = argVars
        self.localVars = sorted(localVars - set(argVars) - set(globalOverrides))
        print >> E, 'Compiler init:', 'parent', parentCompiler, 'argVars', self.argVars, 'localVars', self.localVars, 'globalOverrides', globalOverrides, 'tclass', tclass, 'isDunderInit', isDunderInit

        self.ops = [0, 0, 0, 255, 255, 255]
        self.interns = {}
        self.globals = {}
        self.classes = {}
        self.funcs = {}
        self.tempVars = []  # not used yet
        self.continue_to = None
        self.break_patches = None

    def AddIntern(self, s):
        ## self.interns :: s -> (i, patches)
        if is_in(s, self.interns):
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
        ## Add with intern number.
        ## self.globals :: name -> (j, patches)
        self.AddIntern(name)  # Will need it at Output time.
        if is_in(name, self.globals):
            j, patches = self.globals[name]
            return j
        z = len(self.globals)
        self.globals[name] = (z, [])
        return z

    def PatchGlobal(self, name, patch):
        j, patches = self.globals[name]
        patches.append(patch)
        return j

    def visitExprAndDrop(self, t):
        t.x.visit(self)
        self.ops.append('Drop')

    def visitAssign(self, t):
        t.y.visit(self)
        self.assignTo(t.x)

    def assignToIdent(self, a):
            var = a.x
            if var=='_':
                self.ops.append('Drop')
            elif is_in(var, self.argVars):
                self.ops.append('ArgPut')
                self.ops.append(self.argVars.index(var))
            elif is_in(var, self.localVars):
                self.ops.append('LocalPut')
                self.ops.append(self.localVars.index(var))
            else:
                self.AddGlobal(var)
                self.ops.append('GlobalPut')
                g = self.PatchGlobal(var, len(self.ops))
                self.ops.append(g)
    def assignToMember(self, a):
            if type(a.x) == TIdent and a.x.x == 'self' and self.tclass:
                self.ops.append('SelfMemberPut')
                self.ops.append(sorted(self.tclass.fields).index(a.member))
            else:
                a.x.visit(self)
                self.ops.append('MemberPut')
                isn = self.PatchIntern(a.member, len(self.ops))
                self.ops.append(isn)
    def assignTo(self, a):
        if type(a) is TIdent:
            self.assignToIdent(a)
        elif type(a) is TGetItem:
            a.coll.visit(self)
            a.key.visit(self)
            self.ops.append('PutItem')
        elif type(a) is TMember:
            self.assignToMember(a)
        elif type(a) is TTuple:
            self.ops.append('Explode')
            self.ops.append(len(a.vec))
            for b in a.vec:
                self.assignTo(b)
        else:
            raise Exception('assignTo: bad lhs: %s' % a)

    def visitFunCall(self, t):
        ## fn, xlist
        for x in reversed(t.xlist):  # Iterate in reverse.
            x.visit(self)
        if type(t.fn) == TMember:
            t.fn.x.visit(self)
            self.ops.append('CallMeth')
            isn = self.PatchIntern(t.fn.member, len(self.ops))
            self.ops.append(isn)
            self.ops.append(len(t.xlist) + 1)  # +1 for self.
        else:
            self.visitFunCallMore(t)

    def visitFunCallMore(self, t):
        if type(t.fn) == TIdent and t.fn.x == 'len' and len(t.xlist) == 1:
            self.ops.append('Len')
        elif type(t.fn) == TIdent and t.fn.x == 'chr' and len(t.xlist) == 1:
            self.ops.append('Chr')
        elif type(t.fn) == TIdent and t.fn.x == 'ord' and len(t.xlist) == 1:
            self.ops.append('Ord')
        elif type(t.fn) == TIdent and t.fn.x == 'range' and len(t.xlist) == 1:
            self.ops.append('Range')
        elif type(t.fn) == TIdent and t.fn.x == 'int' and len(t.xlist) == 1:
            self.ops.append('Int')
        elif type(t.fn) == TIdent and t.fn.x == 'str' and len(t.xlist) == 1:
            self.ops.append('Str')
        elif type(t.fn) == TIdent and t.fn.x == 'repr' and len(t.xlist) == 1:
            self.ops.append('Repr')
        elif type(t.fn) == TIdent and t.fn.x == 'sorted' and len(t.xlist) == 1:
            self.ops.append('Sorted')
        elif type(t.fn) == TIdent and t.fn.x == 'ogc' and len(t.xlist) == 0:
            self.ops.append('GC')
        else:
            self.visitFunCallMoreMore(t)

    def visitFunCallMoreMore(self, t):
        if type(t.fn) == TIdent and t.fn.x == 'open' and len(t.xlist) == 2:
            self.ops.append('Open')
        else:
            t.fn.visit(self)
            self.ops.append('Call')
            self.ops.append(len(t.xlist))

    def visitIf(self, t):
        endmarks = []
        for p, b in zip(t.plist, t.blist):
            pass ### TODO -- optimize

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

    def visitAndOr(self, t):
        patches = []
        ## Push the short-circuit value: True for or, False for and.
        self.ops.append('LitInt')
        self.ops.append(1 if t.op=='or' else 0)
        for e in t.vec:
            e.visit(self)
            self.ops.append('BranchIfTrue' if t.op=='or' else 'BranchIfFalse')
            patches.append(len(self.ops))  # request patching.
            self.ops.append(0)  # to be patched.
        ## Invert the short-circuit value, if we reach the end.
        self.ops.append('Not')
        ## Patch short-circuit branches to here at the end.
        for p in patches:
            self.ops[p] = len(self.ops)

    def visitCond(self, t):
        t.cond.visit(self)
        self.ops.append('BranchIfFalse')
        patch_to_no = len(self.ops)
        self.ops.append(0)

        t.yes.visit(self)
        self.ops.append('Branch')
        patch_to_end = len(self.ops)
        self.ops.append(0)

        self.ops[patch_to_no] = len(self.ops)
        t.no.visit(self)
        self.ops[patch_to_end] = len(self.ops)

    def visitUnaryOp(self, t):
        t.x.visit(self)
        if t.op == 'not':
            self.ops.append('Not')
        elif t.op == '-':
            self.ops.append('Negate')
        else:
            raise Exception('bad_unary: %s' % t.op)

    def visitBinaryOp(self, t):
        t.x.visit(self)
        t.y.visit(self)
        if t.op == '+':
            self.ops.append('Plus')
        elif t.op == '-':
            self.ops.append('Minus')
        elif t.op == '*':
            self.ops.append('Times')
        elif t.op == '%':
            self.ops.append('Percent')
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
            self.visitBinaryOpMore(t)
    def visitBinaryOpMore(self, t):
        if t.op == 'is':
            self.ops.append('Is')
        elif t.op == '<<':
            self.ops.append('ShiftLeft')
        elif t.op == '>>':
            self.ops.append('ShiftRight')
        elif t.op == '&':
            self.ops.append('BitAnd')
        elif t.op == '|':
            self.ops.append('BitOr')
        elif t.op == '^':
            self.ops.append('BitXor')
        else:
            raise Exception('visitBinaryOp: bad %s' % t.op)

    def visitMember(self, t):
        if type(t.x) is TIdent and t.x.x == 'sys':
            if t.member == 'stdin':
                self.ops.append('SpecialStdin')
            elif t.member == 'stdout':
                self.ops.append('SpecialStdout')
            elif t.member == 'stderr':
                self.ops.append('SpecialStderr')
            else:
                raise Exception('bad_sys')
        elif type(t.x) is TIdent and t.x.x == 'self' and self.tclass:
            print >>E, 'C FF', self.tclass.fields
            print >>E, 'M', t.member
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
            if is_in(var, self.argVars):
                self.ops.append('ArgGet')
                self.ops.append(self.argVars.index(var))
            elif is_in(var, self.localVars):
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

    def visitSpecial(self, t):
        if t.x=='True':
            self.ops.append('SpecialTrue')
        elif t.x=='False':
            self.ops.append('SpecialFalse')
        elif t.x=='None':
            self.ops.append('SpecialNone')
        ## elif t.x=='Stdin':
        ##     self.ops.append('SpecialStdin')
        ## elif t.x=='Stdout':
        ##     self.ops.append('SpecialStdout')
        ## elif t.x=='Stderr':
        ##     self.ops.append('SpecialStderr')
        else:
            raise t
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
        if len(z) > 250:
            print >>E, 'WARNING: Bytecodes too long: %d' % len(z)
        return z

    def OutputCodePack(self, w):
        print >>E, '(((((((((('
        P.put_str(w, T.CodePack_bytecode, self.OpList2Bytecodes(self.ops))
        i_strs = self.OutputInterns(w)
        self.OutputGlobals(w)
        self.OutputFuncPacks(w, i_strs)
        self.OutputClassPacks(w, i_strs)
        P.put_finish_message(w)
        print >>E, '))))))))))'

    def OutputInterns(self, w):
        ## sort by interns by number i, and write to protobuf in that order.
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
        return i_strs

    def OutputGlobals(self, w):
        ## sort by globals by number j, and write to protobuf in that order.
        g_vec = []
        for name, (j, patches) in self.globals.items():
            g_vec.append((j, name, patches))
        for j, name, patches in sorted(g_vec):
            P.put_start_message(w, T.CodePack_globals)
            P.put_int(w, T.GlobalPack_name_i, self.AddIntern(name))
            for e in patches:
                P.put_int(w, T.GlobalPack_patch, e)
            P.put_finish_message(w)

    def OutputFuncPacks(self, w, i_strs):
        ## funcpacks
        for name, fc in sorted(self.funcs.items()):
            print >>E, 'Func Pack: ((((( name=', name
            P.put_start_message(w, T.CodePack_funcpacks)
            P.put_int(w, T.FuncPack_name_i, i_strs.index(name))

            P.put_start_message(w, T.FuncPack_pack)
            fc.OutputCodePack(w)

            P.put_finish_message(w)  # finish CodePack_funcpacks
            print >>E, 'Func Pack: ))))) name=', name

    def OutputClassPacks(self, w, i_strs):
        ## classpacks
        for name, tclass in sorted(self.classes.items()):
            print >>E, 'Class Pack: ((((((( name=', name
            P.put_start_message(w, T.CodePack_classpacks)
            P.put_int(w, T.ClassPack_name_i, i_strs.index(name))

            for fieldName in tclass.fields:
                isn = self.AddIntern(fieldName)
                P.put_int(w, T.ClassPack_field_i, isn)

            for methName, fc in sorted(tclass.funcs.items()):
                self.OutputMethod(w, i_strs, methName, fc)
                ## print >>E, 'Method: ((( name=', methName
                ## P.put_start_message(w, T.ClassPack_meth)

                ## P.put_int(w, T.FuncPack_name_i, i_strs.index(methName))
                ## P.put_start_message(w, T.FuncPack_pack)
                ## fc.OutputCodePack(w)
                ## P.put_finish_message(w)  # finish ClassPack_meth
                ## print >>E, 'Method: ))) name=', methName

            P.put_finish_message(w)  # finish CodePack_classpacks
            print >>E, 'Class Pack: ))))))) name=', name

    def OutputMethod(self, w, i_strs, methName, fc):
                print >>E, 'Method: ((( name=', methName
                P.put_start_message(w, T.ClassPack_meth)

                P.put_int(w, T.FuncPack_name_i, i_strs.index(methName))
                P.put_start_message(w, T.FuncPack_pack)
                fc.OutputCodePack(w)
                P.put_finish_message(w)  # finish ClassPack_meth
                print >>E, 'Method: ))) name=', methName

#if BIG

    def visitBlock(self, t):
        print >> E, '//block{{{'
        for e in t.vec:
            e.visit(self)
            print >> E
        print >> E, '//block}}}'

    def visitPrint(self, t):
        for e in t.vec:
            e.visit(self)
            self.ops.append('SimplePrint')

    def visitAssert(self, t):
        t.pred.visit(self)

        if t.msg:
            self.ops.append('BranchIfTrue')
            patch_branch = len(self.ops)
            self.ops.append(0)
        
            t.msg.visit(self)
            self.ops.append('SimplePrint')

            self.ops.append('SpecialFalse')
        self.ops.append('Assert')

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
        ## First pass, pure functions only, no globals.
        ## -- name, varlist==arglist, block.
        lg = AssignmentVisitor()
        lg.localVars.update(t.arglist)  # args are localVars.
        print >>E, 'visitDef: t.arglist:', t.arglist
        print >>E, 'visitDef: lg.localVars:', lg.localVars
        lg.visitBlock(t.block)
        print >>E, 'visitDef: ++ lg.localVars:', lg.localVars

        ##// def __init__(self, parentCompiler, argVars, localVars, globalOverrides, tclass, isDunderInit):
        fc = Compiler(self, t.arglist, lg.localVars, lg.globalOverrides, tclass, t.name == '__init__')
        fc.visitBlock(t.block)
        if not len(fc.ops) or (fc.ops[-1] != 'Return' and fc.ops[-1] != 'RetNone' and fc.ops[-1] != 'RetSelf'):
            fc.ops.append('RetSelf' if fc.isDunderInit else 'RetNone')
        func_dict[t.name] = fc

    def visitFor(self, t):
        coll = t.coll.visit(self)
        self.ops.append('CallMeth')
        isn = self.PatchIntern('__iter__', len(self.ops))
        self.ops.append(isn)
        self.ops.append(1)  # one arg (self) to `__iter__`
        self.ops.append('LocalPut')
        self.ops.append(self.localVars.index(t.iterTemp))

        self.ops.append('Try')
        patch_try = len(self.ops)
        self.ops.append(0)  # to the Catch.

        while_top = len(self.ops)
        self.ops.append('LocalGet')
        self.ops.append(self.localVars.index(t.iterTemp))
        self.ops.append('CallMeth')
        isn = self.PatchIntern('next', len(self.ops))
        self.ops.append(isn)
        self.ops.append(1)  # one arg (self) to `next`

        self.assignTo(t.dest)
        prev_continue_to = self.continue_to
        self.continue_to = while_top
        self.break_patches = []
        t.block.visit(self)
        self.ops.append('Branch')
        self.ops.append(while_top)

        self.ops.append('EndTry')  # NOT REACHED.
        patch_end_try = len(self.ops)  # NOT USED.
        self.ops.append(0)  # to the end.

        self.ops[patch_try] = len(self.ops)
        self.ops.append('Catch')
        self.ops.append('Dup')  # create an extra copy

        TStr('StopIteration').visit(self)
        self.ops.append('NE')  # cool if it is StopIteration

        self.ops.append('BranchIfFalse') # continue at the final Drop.
        patch_branch = len(self.ops)
        self.ops.append(0)  # to the end.

        self.ops.append('Raise') # not StopIteration: raise that extra copy

        self.ops[patch_branch] = len(self.ops)
        self.ops.append('Drop')  # drop the extra copy.

        self.ops[patch_end_try] = len(self.ops)  # to the end.
        for bp in self.break_patches:
            self.ops[bp] = len(self.ops)  # to the end.

    def visitTry(self, t):
        self.ops.append('Try')
        patch_try = len(self.ops)
        self.ops.append(0)  # to the Catch.

        t.try_block.visit(self)

        self.ops.append('EndTry')
        patch_catch = len(self.ops)
        self.ops.append(0)  # to the end.

        self.ops[patch_try] = len(self.ops)
        self.ops.append('Catch')     # Catch is a courtesy no-op.
        self.assignTo(t.except_var)  # Assigns exception object to right var.

        t.catch_block.visit(self)
        self.ops[patch_catch] = len(self.ops)


    def visitBreak(self, t):
        if self.break_patches is None:
            raise Exception('bad_break')
        self.ops.append('Branch')
        self.break_patches.append(len(self.ops))
        self.ops.append(0)

    def visitContinue(self, t):
        if self.continue_to is None:
            raise Exception('bad_continue')
        self.ops.append('Branch')
        self.ops.append(self.continue_to)

    def visitWhile(self, t):
        start = len(self.ops)

        t.cond.visit(self)

        self.ops.append('BranchIfFalse')
        patch = len(self.ops)
        self.ops.append(0)  # to the end.

        prev_continue_to = self.continue_to
        self.continue_to = start
        self.break_patches = []
        t.block.visit(self)

        self.ops.append('Branch')
        self.ops.append(start)

        self.ops[patch] = len(self.ops)  # to the end.
        for bp in self.break_patches:
            self.ops[bp] = len(self.ops)  # to the end.

        self.continue_to = prev_continue_to

    def visitGlobal(self, t):
        pass

    def visitRaise(self, t):
        t.ex.visit(self)
        self.ops.append('Raise')

    ## def visitImport(self, t):
    ##     for e in t.vec:
    ##         assert type(e) is TIdent
    ##         TStr(e.x).visit(self)
    ##         ## XXX unimplemented (Import does nothing but drop).
    ##         self.ops.append('Import')


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


class AssignmentVisitor(object):
    def __init__(self):
        self.localVars = set()  # Others are global.
        self.globalOverrides = set()  # Overrides localVars
        self.selfFields = set()

    def visitGlobal(self, t):
        self.globalOverrides.add(t.var)
        print >>E, 'visitGlobal: self.globalVars:', Inside(self.globalOverrides)

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

    def visitExprAndDrop(self, t):
        pass

    def visitFor(self, t):
        print >>E, 'visitFor: * self.localVars:', Inside(self.localVars)
        self.assignTo(t.dest)
        print >>E, 'visitFor: ** self.localVars:', Inside(self.localVars)
        self.localVars.add(t.iterTemp)
        print >>E, 'visitFor: *** self.localVars:', Inside(self.localVars)
        t.block.visit(self)

    def visitAssign(self, t):
        print >>E, 'visitAssign: @ self.localVars:', Inside(self.localVars)
        self.assignTo(t.x)
        print >>E, 'visitAssign: @@ self.localVars:', Inside(self.localVars)

    def assignTo(self, t):
        print >>E, 'assignTo: =======', t
        if type(t) is TIdent:
            self.localVars.add(t.x)
        elif type(t) is TMember and type(t.x) is TIdent and t.x.x == 'self':
            self.selfFields.add(t.member)
        elif type(t) is TMember and type(t.x) is TIdent:
            pass  # Foreign member.
        elif type(t) is TGetItem:
            pass
        elif type(t) is TTuple:
            for e in t.vec:
                self.assignTo(e)
        elif type(t) is list: # TODO does this still occur?
            for e in t:
                self.assignTo(e)
        elif type(t) is str:  # TODO does this still occur?
            self.localVars.add(t)
        else:
            raise Exception('bad target: %s' % t)

    def visitClass(self, t):
        Abort()  # No nested classes.

    def visitDef(self, t, func_dict, tclass):
        Abort()  # No nested functions, yet.

    def visitTry(self, t):
        t.try_block.visit(self)
        t.catch_block.visit(self)
        self.assignTo(t.except_var)

    def visitBreak(self, t):
        pass
    def visitContinue(self, t):
        pass
    def visitWhile(self, t):
        t.block.visit(self)

    def visitIf(self, t):
        for p, b in zip(t.plist, t.blist):
            b.visit(self)
        if t.belse:
            t.belse.visit(self)

    def visitCond(self, t):
        pass
    def visitUnaryOp(self, t):
        pass
    def visitBinaryOp(self, t):
        pass

    def visitIdent(self, t):
        if type(t.x) is str:
            ## assume GlobalDict for now.
            var = t.x
            self.AddGlobal(var)
            self.ops.append('GlobalGet')
            g = self.PatchGlobal(var, len(self.ops))
            self.ops.append(g)
        else:
            raise Exception('visitIdent: bad var: %s %s' % (type(t.x), t.x))

    def visitSpecial(self, t):
        pass
    def visitStr(self, t):
        pass

    def visitInt(self, t):
        pass

#endif

class BytesWriter(object):
    def __init__(self):
        self.v = []

    def append(self, x):
        self.v.append(x)

    def Bytes(self):
        vec = []
        for e in self.v:
            vec.append(chr(e))
        return ''.join(vec)
        ## return ''.join([chr(e) for e in self.v])


class AppendWriter(object):
    def __init__(self, w):
        self.w = w

    def append(self, x):
        self.w.write(chr(x))


if __name__ == '__main__':  # test
#if UNIX
    GetBytecodeNumbers()
#endif

    p = Parser(Stdin.read())
    compiler = Compiler(None, [], set(), set(), None, False)

#if SMALL
    single = p.ParseSingle()
    single.visit(compiler)
    compiler.ops.append('Return')
#endif

#if BIG
    block = p.ParseBlock()
    compiler.visitBlock(block)
    compiler.ops.append('RetNone')
#endif

    compiler.OutputCodePack(AppendWriter(Stdout))

pass
