def RangeDictWithWhile(n):
    i = 0
    z = {}
    while i < n:
        z[i] = (i, i)
        i = i + 1
    return z

def Sum(coll):
    total = 0
    for k in coll:
        total = total + k
    return total

d = RangeDictWithWhile(11)
print d
assert 55 == Sum(d)

v = range(101)
print v
x = Sum(v)
print x
assert x == 5050
