import sys

Classes = []
Macros = []
Funcs = []


def DefineEnum(name, members):
    print 'enum %sNumbers {' % name
    i = 0
    for m in members:
        print '  %s = %d,' % (m, i)
        i += 1
    print '};'

    print 'extern const char* const %sNames[];' % name
    print 'extern word %sNames_SIZE;' % name


def DefineEnumNames(name, members):
    print 'const char* const %sNames[] = {' % name
    i = 0
    for m in members:
        print '  "%s", // %d' % (m, i)
        i += 1
    print '};'
    print 'word %sNames_SIZE = %d;' % (name, len(members))


class ClassDef(object):
    def __init__(self, name, num):
        self.name = name
        self.num = num
        self.fields = []

    def __str__(self):
        z = 'CLS(%s)' % self.name
        for t, s in self.fields:
            z += ' %s:%s;' % (t, s)
        return z


def CompileCore(core_contents):
    kind = None
    topic = None
    class_enums = []
    code_enums = []
    code_arg = {}
    code_in = {}
    code_out = {}
    code_body = {}
    prim_enums = []
    message_enums = []
    class_message_to_meth = dict()
    in_body = False
    body = ''
    for line in core_contents.split('\n'):
        if in_body:
            if line.strip() == ']':
                code_body[topic] = body
                in_body = False
                continue
            body += line + '\n'
            continue
        line = line.split('#')[0]
        words = line.split()
        if not words: continue
        elif words[0] == 'code':
            kind = 'c'
            topic = 'BC_%s' % words[1]
            code_enums.append(topic)
            code_arg[topic] = []
            code_in[topic] = []
            code_out[topic] = []
        elif words[0] == 'prim':
            kind = 'm'
            topic = 'PRIM_%s' % '_'.join(words[1:])
            topic_cls = words[1]
            topic_message = words[2]

            if topic_message not in message_enums:
                message_enums.append(topic_message)

            if topic_cls not in class_message_to_meth:
                class_message_to_meth[topic_cls] = dict()
            class_message_to_meth[topic_cls][topic_message] = len(prim_enums)
            prim_enums.append(topic)

            code_in[topic] = []
            code_out[topic] = []
        elif words[0] == 'arg':
            code_arg[topic] = words[1:]
        elif words[0] == 'in':
            code_in[topic] = words[1:]
        elif words[0] == 'out':
            code_out[topic] = words[1:]
        elif words[0] == '[':
            body = ''
            in_body = True
        elif words[0] == 'class':
            kind = 'class'
            topic = ClassDef(words[1], len(Classes))
            Classes.append(topic)
            class_enums.append('C_%s' % words[1])
        elif words[0] in ['oop', 'small', 'byte[]', 'oop[]']:
            topic.fields.append(words)
        else:
            raise Exception('unknown def: %s' % repr(words))

    for c in Classes:
        at = 0
        for ftype, fname in c.fields:
            if ftype == 'small':

                Macros.append('%s_%s(P) TO_INT(ogetw((P)+%d))' % (c.name,
                                                                  fname, at))
                Macros.append('%s_%s_Start(P,X) oputw((P)+%d, FROM_INT(X))' %
                              (c.name, fname, at))
                Macros.append('%s_%s_Put(P,X) oputw((P)+%d, FROM_INT(X))' %
                              (c.name, fname, at))

            elif ftype == 'oop':

                Macros.append('%s_%s(P) ogetw((P)+%d)' % (c.name, fname, at))
                Macros.append('%s_%s_Put(P,X) oputw((P)+%d, (X))' %
                              (c.name, fname, at))
                Macros.append('%s_%s_LEA(P) ((P)+%d)' % (c.name, fname, at))

            elif ftype == 'byte[]':

                Macros.append('%s_%s_At(P,I) ogetb((word)(P)+%d+(byte)(I))' %
                              (c.name, fname, at))
                Macros.append(
                    '%s_%s_AtPut(P,I,X) oputb((word)(P)+%d+(byte)(I),(byte)(X))'
                    % (c.name, fname, at))

            elif ftype == 'oop[]':

                Macros.append(
                    '%s_%s_At(P,I) ogetw((word)(P)+%d+((byte)(I)<<1))' %
                    (c.name, fname, at))
                Macros.append(
                    '%s_%s_AtPut(P,I,X) oputw((word)(P)+%d+((byte)(I)<<1),(word)(X))'
                    % (c.name, fname, at))

            at += 2
        Macros.append('%s_Size %d' % (c.name, at))

    print '// THIS FILE IS GENERATED'
    print
    print '#ifndef _CORE_DECLARATIONS_'
    print '#define _CORE_DECLARATIONS_'
    print
    print '#include "octet.h"'
    print '#include "defs.h"'
    print

    DefineEnum('Class', class_enums)
    print
    DefineEnum('Code', code_enums)
    print
    DefineEnum('Message', message_enums)
    print
    DefineEnum('Prim', prim_enums)
    print

    for m in Macros:
        print '#define %s' % m
    print
    for head, _ in Funcs:
        print 'extern %s;' % head

    print 'extern const byte Prims[];'
    print
    print '#endif // _CORE_DECLARATIONS_'
    print
    print '#ifdef PRIM_PART'
    print
    print '#if PRIM_PART == 2'
    DefineEnumNames('Class', class_enums)
    DefineEnumNames('Code', code_enums)
    DefineEnumNames('Message', message_enums)

    print
    print 'const byte Prims[] = {'
    for topic_cls, d1 in class_message_to_meth.items():
        for topic_message, mess_num in d1.items():
            print '  C_%s, %s, // METH_%s_%s == %d' % (topic_cls,
                                                       topic_message,
                                                       topic_cls,
                                                       topic_message, mess_num)
    print '  0, 0};'
    print

    for head, body in Funcs:
        print '%s %s' % (head, body)
    print '#endif // if PRIM_PART == 2'
    print
    print '////////////////////////////////'
    print
    print '#if PRIM_PART == 3'
    print
    for c in code_enums:
        print '// CASE:', c, repr(code_arg[c]), repr(code_in[c]), repr(
            code_out[c])
        print 'case %s: {' % c
        i = len(code_in[c])
        for e in code_in[c]:
            i -= 1
            print '  word %s = ogetw(sp + %d);  /*in*/' % (e, i + i)
        i = 0
        for e in code_out[c]:
            i += 1
            print '    word %s = 0; // out' % e
        print '{'
        for e in code_arg[c]:
            print '  byte %s = ogetb(ip++);' % e
        print code_body[c]
        print '}'
        pops = len(code_in[c]) - len(code_out[c])
        if pops > 0:
            for i in range(pops):
                print 'oputw(sp, 0xDEAD);'
                print 'sp += 2;'
        if pops < 0:
            print 'sp -= %d;' % (-2 * pops)

        for e in code_in[c]:
            pass
        i = len(code_out[c])
        for e in code_out[c]:
            i -= 1
            print 'oputw(sp + %d, %s); // out' % (2 * i, e)
        print '} goto RUN_LOOP;'
        print ''
        print ''
    print '#endif // if PRIM_PART == 3'
    print
    print '////////////////////////////////'
    print
    print '#if PRIM_PART == 4'
    print
    for c in prim_enums:
        print '// CASE:', c, repr(code_in[c]), repr(code_out[c])
        print 'case %s: {' % c
        i = len(code_in[c])
        for e in code_in[c]:
            i -= 1
            print '  word %s = ogetw(sp + %d);  /*in*/' % (e, i + i)
        i = 0
        for e in code_out[c]:
            i += 1
            print '    word %s = 0; // out' % e
        print '{'
        print code_body[c]
        print '}'
        pops = len(code_in[c]) - len(code_out[c])
        if pops > 0:
            for i in range(pops):
                print 'oputw(sp, 0xDEAD);'
                print 'sp += 2;'
        if pops < 0:
            print 'sp -= %d;' % (-2 * pops)

        for e in code_in[c]:
            pass
        i = len(code_out[c])
        for e in code_out[c]:
            i -= 1
            print 'oputw(sp + %d, %s); // out' % (2 * i, e)
        print '} break;'
        print ''
        print ''
    print '#endif // if PRIM_PART == 4'
    print
    print '#endif // ifdef PRIM_PART'
    print
    i = 0
    for c in code_enums:
        print '// :ByteCode: %s %d %s' % (c, i, code_arg[c])
        i += 1
    print
    print '//', repr(class_message_to_meth)
    print
    print '// THIS FILE IS GENERATED'


CompileCore(sys.stdin.read())
