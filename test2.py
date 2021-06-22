x = 100
y = 5 + x
z = x + y - 205
assert 0 == z
if z:
    print z
else:
    print 42  # <-- 42
if x:
    print x  # <-- 100
else:
    print 13
if z:
    print z
elif y:
    print y  # <-- 105
else:
    print 13
