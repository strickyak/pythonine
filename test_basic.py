import sys
E = sys.stderr

def Add(a, b): return a+b
def Sub(a, b): return a-b
def Mul(a, b): return a*b
def Div(a, b): return a/b
def Mod(a, b): return a%b
def EQ(a, b): return a==b
def LT(a, b): return a<b
def GT(a, b): return a>b

BinaryOps = {}
def initBinaryOps():
  BinaryOps['+'] = Add
  BinaryOps['-'] = Sub
  BinaryOps['*'] = Mul
  BinaryOps['/'] = Div
  BinaryOps['%'] = Mod
  BinaryOps['='] = EQ
  BinaryOps['<'] = LT
  BinaryOps['>'] = GT
initBinaryOps()

# Expressions

class XInt:
    def __init__(self, num):
        self.num = num

    def __str__(self):
        return str(self.num)

    def eval(self):
        return self.num

class XVar:
    def __init__(self, var):
        self.var = var

    def __str__(self):
        return self.var

    def eval(self):
        try:
            return Vars[self.var]
        except:
            return 0


class XBinary:
    def __init__(self, a, op, b):
        self.a = a
        self.op = op
        self.b = b

    def __str__(self):
        return "(" + self.a.__str__() + self.op + self.b.__str__() + ")"

    def eval(self):
        a = self.a.eval()
        b = self.b.eval()
        fn = BinaryOps[self.op]
        z = fn(a, b)
        return z


class XCall:
    def __init__(self, fn, args):
        self.fn = fn
        self.args = args

    def __str__(self):
        return str(self.fn) + "(" + str(self.args) + ")"

    def eval(self):
        raise Exception('TODO')


def List():
    print chr(13)
    for (num, stmt) in sorted(Program.items()):
        if stmt:
            print num, stmt.__str__(), chr(13)

def Run():
    LineNum[0] = 1
    while True:
        x, y = Nearest(LineNum[0])
        ## print '[',x,y,']'
        if x <= 0:
            break
        LineNum[0] = x + 1
        y.execute()
    print '[STOP]'

def Nearest(n):
    x, y = 15000, 0
    for (num, stmt) in Program.items():
        if num >= n and num < x:
            x, y = num, stmt
    if x == 15000:
        x = 0
    return x, y


# Lexical Token types.
T_EOL = 0
T_INT = 1
T_STR = 2
T_WORD = 3
T_ETC = 4

class Parser(object):
    def __init__(self, line):
        self.line = line
        self.i = 0
        self.n = len(line)
        self.t = None
        self.x = None

    def Lex(self):
        self.Lex_1()

    def Lex_1(self):
        while True:
            if self.i >= self.n:
                self.t, self.x = T_EOL, None
                return
            c = self.line[self.i]
            if c == '#':
                self.i = self.n
                self.t, self.x = T_EOL, None
                return
            if c <= ' ':
                self.i = self.i + 1
                continue
            break
        self.Lex_2(c)

    def Lex_2(self, c):
            if '0' <= c and c <= '9':
                x = 0
                while '0' <= c and c <= '9':
                    x = 10 * x + ord(c) - ord('0')
                    self.i = self.i + 1
                    if self.i >= self.n: break
                    c = self.line[self.i]
                self.t, self.x = T_INT, x
                return
            self.Lex_3(c)
    def Lex_3(self, c):
            if 'A' <= c and c <= 'Z' or 'a' <= c and c <= 'z':
                s = ''
                while 'A' <= c and c <= 'Z' or 'a' <= c and c <= 'z':
                    s = s + c
                    self.i = self.i + 1
                    if self.i >= self.n: break
                    c = self.line[self.i]
                self.t, self.x = T_WORD, s.upper()
                return
            # other punc
            self.t, self.x = T_ETC, c
            self.i = self.i + 1
            return

    def parsePrim(self):
        first = self.x
        if self.t == T_INT:
            self.Lex()
            return XInt(int(first))
        if self.t == T_WORD:
            self.Lex()
            return XVar(first)
        if self.t == T_ETC and first == '(':
            self.Lex()
            a = self.parseExpr()
            if self.t != T_ETC or self.x != ')':
                raise Exception('got %s want )' % self.x)
            self.Lex()
            return a

    def parsePrimEtc(self):
        a = self.parsePrim()
        while self.t == T_ETC and self.x == '(':
            self.Lex()
            args = []
            while self.t != T_ETC or self.x != ')':
                args.append(self.parseExpr())
                if self.t == T_ETC and self.x == ',':
                    self.Lex()
            a = XCall(a, args)
        return a

    def parseProduct(self):
        a = self.parsePrim()
        t, op = self.t, self.x
        while t == T_ETC and (op=='*' or op=='%' or op=='/'):
            self.Lex()
            b = self.parsePrim()
            a = XBinary(a, op, b)
            t, op = self.t, self.x
        return a

    def parseSum(self):
        a = self.parseProduct()
        t, op = self.t, self.x
        while t == T_ETC and (op=='+' or op=='-'):
            self.Lex()
            b = self.parseProduct()
            a = XBinary(a, op, b)
            t, op = self.t, self.x
        return a

    def parseRel(self):
        a = self.parseSum()
        t, op = self.t, self.x
        while t == T_ETC and (op=='<' or op=='=' or op=='>'):
            self.Lex()
            b = self.parseSum()
            a = XBinary(a, op, b)
            t, op = self.t, self.x
        return a

    def parseExpr(self):
        return self.parseRel()

    def parseLET(self):
        self.Lex()
        if self.t != T_WORD:
            raise Exception('let_wants_var')
        var = self.x
        self.Lex()
        if self.x != "=":
            raise Exception('let_wants_eq')
        self.Lex()
        a = self.parseExpr()
        return LET(var, a)

    def parseREM(self):
        s = ''
        while self.i < self.n:
            s = s + self.line[self.i]
            self.i = self.i + 1
        return REM(s)

    def parsePRINT(self):
        self.Lex()
        a = None
        if self.t != EOL:
            a = self.parseExpr()
        return PRINT(a, None)

    def parseGOTO(self):
        self.Lex()
        a = self.parseExpr()
        return GOTO(a)

    def parseIF(self):
        self.Lex()
        pred = self.parseExpr()
        if self.x != "THEN":
            raise Exception('if_wants_then')
        self.Lex()
        target = self.parseExpr()
        return IF(pred, target)

    def parseStatementOrNone(self):
        if self.t == T_EOL:
            return None
        if self.t != T_WORD:
            raise Exception('syntax_err')
        if self.x == "PRINT" or self.x == "P":
            return self.parsePRINT()
        if self.x == "LET":
            return self.parseLET()
        if self.x == "GOTO" or self.x == "G":
            return self.parseGOTO()
        if self.x == "IF":
            return self.parseIF()
        if self.x == "REM":
            return self.parseREM()
        raise Exception(("unknown_stmt", self.t, self.x))

    def parseNumberedStatement(self):
        n, st = 0, None
        if self.t == T_INT:
            n = int(self.x)
            self.Lex()
            if self.t != T_EOL:
                st = self.parseStatementOrNone()
        return n, st

def Insert(n, st):
    Program[n] = st


# Statements

class REM:
    def __init__(self, remark):
        self.remark = remark

    def __str__(self):
        return 'REM ' + self.remark

    def execute(self):
        pass

class LET:
    def __init__(self, var, expr):
        self.var = var.upper()
        self.expr = expr

    def __str__(self):
        return 'LET ' + self.var + ' = ' + self.expr.__str__()

    def execute(self):
        Vars[self.var] = self.expr.eval()

class PRINT:
    def __init__(self, expr, tail):
        self.expr = expr
        self.tail = tail

    def __str__(self):
        if self.expr == None:
            return 'PRINT '
        else:
            return 'PRINT ' + self.expr.__str__()

    def execute(self):
        if self.expr == None:
            print chr(10)
        else:
            print self.expr.eval(),
        sys.stdout.flush()


class GOTO:
    def __init__(self, expr):
        self.expr = expr

    def __str__(self):
        return 'GOTO ' + self.expr.__str__()

    def execute(self):
        LineNum[0] = self.expr.eval()
        # ogc()


class IF:
    def __init__(self, pred, target):
        self.pred = pred
        self.target = target

    def __str__(self):
        return 'IF ' + self.pred.__str__() + ' THEN ' + self.target.__str__()

    def execute(self):
        if self.pred.eval():
            LineNum[0] = self.target.eval()
        # ogc()

Vars = {}
Program = {}
LineNum = [1]

def Command(line):
    try:
        p = Parser(line)
        p.Lex()
        if p.t == T_INT:
            n, st = p.parseNumberedStatement()
            Insert(n, st)
        else:
            st = p.parseStatementOrNone()
            if st:
                st.execute()
            else:
                raise Exception('not_a_statement')
    except Exception as ex:
        print " *** ERR", ex


def Loop():
    while True:
        # ogc()
        print "  -->",
        line = sys.stdin.readline()
        if not line: break

        line = line.rstrip().upper()
        if line == "Q" or line == "B" or line == "BYE": break
        elif line == "L" or line == "LIST": List()
        elif line == "R" or line == "RUN": Run()
        elif line == "SHELL": shell()
        else: Command(line)

def Init():
    Command("10 REM Prove the Collatz Conjecture")
    Command("30 LET a = 0                # Loop for natural nums with A")
    Command("50 LET a = a + 1")
    Command("80 PRINT")
    Command("100 LET b = a               # start testing a chain with A")
    Command("110 PRINT b")
    Command("120 IF b < 2 THEN 50        # next a")
    Command("130 IF b % 2 = 0 THEN 500")
    Command("200 LET b = 3 * b + 1       # if odd")
    Command("220 GOTO 110")
    Command("500 LET b = b / 2           # if even")
    Command("520 GOTO 110")

Init()
Loop()
print "BYE!"
