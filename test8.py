x = [10, 20, 88, 99]
d = {10: 110, 20: 120, 30: 130}
t = ((3, 4), 55)
print x
print d
print t
assert 4 == len(x)
assert 3 == len(d)
assert 2 == len(t)
assert 88 == x[2]
assert 120 == d[20]
assert 55 == t[1]
(p1, p2), q = t
assert p1 == 3
assert p2 == 4
assert q == 55

#assert '0xfa' == hex(250)
assert 'E' == chr(69)
assert ord('Z') == 64 + 26
pass
