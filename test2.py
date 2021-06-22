x = 100
y = 5 + x
z = x + y - 205
assert 0 == z
if z:
    print z
    assert 0
else:
    print 42  # <-- 42
    assert 1
if x:
    print x  # <-- 100
    assert 1
else:
    print 13
    assert 0
if z:
    print z
    assert 0
elif y:
    print y  # <-- 105
    assert 1
else:
    print 13
    assert 0

sum = 0
i = 0
while i <= 6:
    sum = sum + i
    i = i + 1
print sum
assert sum == 21
