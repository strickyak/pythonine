x = [10, 20, 88, 99]
d = {10: 110, 20: 120, 30: 130}
print x
print d
assert 4 == len(x)
assert 3 == len(d)
assert 88 == x[2]
assert 120 == d[20]

#assert '0xfa' == hex(250)
assert 'E' == chr(69)
assert ord('Z') == 64 + 26
pass
