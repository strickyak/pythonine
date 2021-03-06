import sys
E = sys.stderr

D = {}

print( sys.argv, file=E)
for const_file in sys.argv[1:]:
    print( const_file, file=E)
    with open(const_file) as r:
        for line in r:
            line = line.rstrip()
            key, value = line.split('=')
            D[key] = value

for line in sys.stdin:
    line = line.rstrip()
    for k, v in D.items():
        line = line.replace(k, v)
    print( line)
