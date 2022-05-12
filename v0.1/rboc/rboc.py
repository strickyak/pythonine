import sys, re
from os import path
from collections import defaultdict

# Contains the string "rboc" at the end of the line.
HAS_RBOC = re.compile(R'rboc\s*$')

# Full parsing of "rettype FuncName(type1 arg1, type2 args); // rboc"
CAPTURE_RBOC_DECL = re.compile(R'(.*)[(](.*)[)]\s*;\s*//\s*rboc')

CAPTURE_FINAL_NAME = re.compile(R'^(.*)\b(\w+)$')

# Regions [ regionInt ] = [ ( funcName, retTyp, [(argtype, argname)] ]
Regions = defaultdict(list)

# Arguments are like "region:headername.h"
for x in sys.argv[1:]:
    region, filename = x.split(':')
    region = int(region)
    for line in open(filename):
        line = line.strip()
        if HAS_RBOC.search(line):
            print('[', line, ']')
            m = CAPTURE_RBOC_DECL.search(line)
            assert m

            ret_and_name = m.group(1).strip()
            args = m.group(2).strip()
            ret, name = CAPTURE_FINAL_NAME.match(ret_and_name).groups()

            if args:
                arglist = [CAPTURE_FINAL_NAME.match(s).groups() for s in args.split(',')]
            else:
                arglist = []
            
            t = ( name, ret, arglist )
            print('made',t)

            Regions[region].append(t)

print(repr(Regions))

Outputs = {}
for origin in [0] + list(Regions.keys()):
    w = open('region.%d.h' % origin, 'w', encoding='ascii')
    Outputs[origin] = w
    print('''
#ifdef FOR_REGION
#error Only include one region file.  Include it last.
----- error ---- Only include one region file.  Include it last.
#endif
#define FOR_REGION %d
''' % origin, file=w)

for region, symbols in Regions.items():
    for name, ret, arglist in symbols:
        print(repr((region, name, ret, arglist)))
        params = [a[1].strip() for a in arglist]

        front = '#define %s(%s)  ' % (name, ','.join(params) if params else '')
        paramspec = ''.join([(', (word)(%s)' % p) for p in params])
        back = '( (%s) CALLER( %d, %s_LOCATION, %d   %s   ) )' % (ret, region, name, len(params), paramspec)

        macro = front + back
        print(macro)
        for origin in [0] + list(Regions.keys()):
            if origin == region: continue

            w=Outputs[origin]
            print(macro, file=w)
            print('#ifdef PASS_ONE', file=w)
            print('#define %s_LOCATION ((word)0x1234) // temporary filler value' % name, file=w)
            print('#endif', file=w)


for f in Outputs.values():
    f.close()

pass # end
