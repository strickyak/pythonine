import sys


def DefineEnum(name, members):
    print 'enum %sNumbers {' % name
    i = 0
    for m in members:
        print '  %s = %d,' % (m, i)
        i += 1
    print '};'

    print 'extern const char* %sNames[];' % name


def DefineEnumNames(name, members):
    print 'const char* %sNames[] = {' % name
    i = 0
    for m in members:
        print '  "%s", // %d' % (m, i)
        i += 1
    print '};'


class ClassDef(object):
    def __init__(self, name, num):
        self.name = name
        self.num = num
        self.is_bytes = False
        self.is_oops = False
        self.fields = []

    def __str__(self):
        z = 'CLS(%s)' % self.name
        if self.is_bytes:
            z += ' bytes;'
        if self.is_oops:
            z += ' oops;'
        for t, s in self.fields:
            z += ' %s:%s;' % (t, s)
        return z


Classes = []
Macros = []
Funcs = []


def CompileCore(core_contents):
    kind = None
    topic = None
    class_enums = []
    code_enums = []
    code_arg = {}
    code_in = {}
    code_out = {}
    code_body = {}
    in_body = False
    body = ''
    for line in core_contents.split('\n'):
        if in_body:
            if line.strip() == '}':
                code_body[topic] = body
                in_body = False
                continue
            body += line + '\n'
            continue
        line = line.split('#')[0]
        words = line.split()
        if not words: continue
        elif words[0] == 'code':
            topic = 'BC_%s' % words[1]
            code_enums.append(topic)
            code_arg[topic] = []
            code_in[topic] = []
            code_out[topic] = []
        elif words[0] == 'arg':
            code_arg[topic] = words[1:]
        elif words[0] == 'in':
            code_in[topic] = words[1:]
        elif words[0] == 'out':
            code_out[topic] = words[1:]
        elif words[0] == '{':
            body = ''
            in_body = True
        elif words[0] == 'class':
            kind = 'class'
            topic = ClassDef(words[1], len(Classes))
            Classes.append(topic)
            class_enums.append('C_%s' % words[1])
        elif words[0] in ['oop', 'small']:
            topic.fields.append(words)
        elif words[0] == 'byte[]':
            topic.is_bytes = True
        elif words[0] == 'oop[]':
            topic.is_oops = True
        else:
            raise Exception('unknown def: %s' % repr(words))

    for c in Classes:
        if c.is_bytes:
            Macros.append('%s_at(P,I) ogetb((word)(P)+(byte)(I))' % c.name)
            Macros.append(
                '%s_at_put(P,I,X) oputb((word)(P)+(byte)(I),(byte)(X))' %
                c.name)
            continue

        if c.is_oops:
            Macros.append('%s_at(P,I) ogetw((word)(P)+(byte)(I))' % c.name)
            Macros.append(
                '%s_at_put(P,I,X) oputw((word)(P)+(byte)(I),(byte)(X))' %
                c.name)
            continue

        if not c.is_bytes and not c.is_oops:
            head = 'word %s_NEW()' % c.name
            body = '{word p=oalloc(%d, C_%s);' % (2 * len(c.fields), c.name)
            offset = 0
            for ftype, fname in c.fields:
                if ftype == 'small':
                    body += 'StartQ(p+%d, 0);' % offset
                offset += 2
            Funcs.append((head, body + ' return p; }'))

        at = 0
        for ftype, fname in c.fields:
            if ftype == 'small':

                Macros.append('%s_%s(P) ogetb((P)+%d)' % (c.name, fname,
                                                          at + 1))
                Macros.append('%s_%s_Start(P,X) StartQ((P)+%d, (X))' %
                              (c.name, fname, at))
                Macros.append('%s_%s_Put(P,X) PutQ((P)+%d, (X))' % (c.name,
                                                                    fname, at))
                Macros.append('%s_%s_LEAQ(P) ((P)+%d)' % (c.name, fname,
                                                          at + 1))

            elif ftype == 'oop':

                Macros.append('%s_%s(P) ogetw((P)+%d)' % (c.name, fname, at))
                Macros.append('%s_%s_Put(P,X) oputw((P)+%d, (X))' %
                              (c.name, fname, at))
                Macros.append('%s_%s_LEA(P) ((P)+%d)' % (c.name, fname, at))

            at += 2

    print "// THIS FILE IS GENERATED"
    print
    print '#ifndef _CORE_DECLARATIONS_'
    print '#define _CORE_DECLARATIONS_'

    DefineEnum('Class', class_enums)
    DefineEnum('Code', code_enums)

    for m in Macros:
        print '#define %s' % m
    for head, _ in Funcs:
        print 'extern %s;' % head

    print
    print '#endif // _CORE_DECLARATIONS_'
    print
    print '#ifdef _CORE_PART_'
    print
    print '#if _CORE_PART_ == 2'
    DefineEnumNames('Class', class_enums)
    DefineEnumNames('Code', code_enums)
    for head, body in Funcs:
        print '%s %s' % (head, body)
    print '#endif // if _CORE_PART_ == 2'
    print
    print '#if _CORE_PART_ == 3'
    for c in code_enums:
        print '// CASE:', c, repr(code_arg[c]), repr(code_in[c]), repr(
            code_out[c])
        print 'case %s:' % c
        i = len(code_in[c])
        for e in code_in[c]:
            i -= 1
            print '#define %s Stack[sp - %d] /*in*/' % (e, i)
        i = 0
        for e in code_out[c]:
            print '#define %s Stack[sp - %d + %d + 1] /*out*/' % (
                e, len(code_in[c]), i)
            i += 1
        print '{'
        for e in code_arg[c]:
            print '  byte %s = codes[++ip];' % e
        print code_body[c]
        print '}'
        pops = len(code_in[c]) - len(code_out[c])
        if pops > 0:
            for i in range(pops):
                print 'Stack[sp--] = 0;'
        if pops < 0:
            print 'sp += %d;' % (-1 * pops)

        for e in code_in[c]:
            print '#undef %s' % e
        for e in code_out[c]:
            print '#undef %s' % e
        print 'break;'
        print ''
        print ''
    print '#endif // if _CORE_PART_ == 3'
    print
    print '#endif // ifdef _CORE_PART_'
    print
    print "// THIS FILE IS GENERATED"


CompileCore(sys.stdin.read())
